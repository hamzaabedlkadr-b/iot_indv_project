#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "comm_lorawan.h"
#include "project_config.h"
#include "project_types.h"

static const char *TAG = "comm_lorawan";

static bool lorawan_path_enabled(const project_context_t *ctx)
{
    return ctx != NULL && project_comm_mode_supports_lorawan(ctx->communication_mode) != 0U;
}

static bool lorawan_radio_tx_enabled(void)
{
    return PROJECT_LORAWAN_ENABLE_RADIO_TX != 0U;
}

static uint16_t clamp_u16_from_float(float value, float scale)
{
    const float scaled_value = value * scale;

    if (scaled_value <= 0.0f) {
        return 0U;
    }
    if (scaled_value >= 65535.0f) {
        return 65535U;
    }
    return (uint16_t)(scaled_value + 0.5f);
}

static int16_t clamp_i16_from_float(float value, float scale)
{
    const float scaled_value = value * scale;

    if (scaled_value <= (float)INT16_MIN) {
        return INT16_MIN;
    }
    if (scaled_value >= (float)INT16_MAX) {
        return INT16_MAX;
    }

    if (scaled_value >= 0.0f) {
        return (int16_t)(scaled_value + 0.5f);
    }

    return (int16_t)(scaled_value - 0.5f);
}

static void encode_u16_be(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[1] = (uint8_t)(value & 0xFFU);
}

static void encode_i16_be(uint8_t *buffer, int16_t value)
{
    encode_u16_be(buffer, (uint16_t)value);
}

static void bytes_to_hex(const uint8_t *bytes,
                         size_t byte_count,
                         char *out_hex,
                         size_t out_hex_size)
{
    static const char s_hex_digits[] = "0123456789ABCDEF";
    const size_t required_size = (byte_count * 2U) + 1U;

    if (out_hex == NULL || out_hex_size < required_size) {
        return;
    }

    for (size_t index = 0; index < byte_count; ++index) {
        out_hex[(index * 2U)] = s_hex_digits[(bytes[index] >> 4) & 0x0FU];
        out_hex[(index * 2U) + 1U] = s_hex_digits[bytes[index] & 0x0FU];
    }

    out_hex[required_size - 1U] = '\0';
}

static transmission_payload_t build_lorawan_message(const aggregate_result_t *aggregate)
{
    // TTN payloads need to stay compact, so we scale the float fields into a fixed 10-byte packet
    // and keep a hex copy around for logs and offline decoder checks.
    uint8_t binary_payload[10];
    char payload_hex[(sizeof(binary_payload) * 2U) + 1U];
    transmission_payload_t message = {
        .created_at_us = (uint64_t)esp_timer_get_time(),
        .window_id = aggregate->window_id,
        .source_timestamp_us = aggregate->window_end_us,
        .publish_latency_us = 0U,
    };

    if (message.created_at_us >= aggregate->window_end_us) {
        message.publish_latency_us = message.created_at_us - aggregate->window_end_us;
    }

    encode_u16_be(&binary_payload[0], (uint16_t)(aggregate->window_id & 0xFFFFU));
    encode_u16_be(&binary_payload[2], (uint16_t)(aggregate->sample_count & 0xFFFFU));
    encode_u16_be(&binary_payload[4], clamp_u16_from_float(aggregate->sampling_frequency_hz, 10.0f));
    encode_u16_be(&binary_payload[6], clamp_u16_from_float(aggregate->dominant_frequency_hz, 100.0f));
    encode_i16_be(&binary_payload[8], clamp_i16_from_float(aggregate->average_value, 1000.0f));
    bytes_to_hex(binary_payload, sizeof(binary_payload), payload_hex, sizeof(payload_hex));

    snprintf(message.topic, sizeof(message.topic), "%s", PROJECT_LORAWAN_UPLINK_TOPIC);
    snprintf(message.payload,
             sizeof(message.payload),
             "{\"device\":\"%s\",\"window_id\":%" PRIu32
             ",\"fport\":%u,\"payload_hex\":\"%s\",\"sample_count\":%" PRIu32
             ",\"signal_profile\":\"%s\",\"anomaly_count\":%" PRIu32
             ",\"sampling_frequency_hz\":%.1f,\"dominant_frequency_hz\":%.2f"
             ",\"average_value\":%.4f,\"created_at_us\":%" PRIu64
             ",\"edge_delay_us\":%" PRIu64 ",\"state\":\"%s\"}",
             PROJECT_DEVICE_NAME,
             aggregate->window_id,
             PROJECT_LORAWAN_FPORT,
             payload_hex,
             aggregate->sample_count,
             project_signal_profile_name(aggregate->signal_profile),
             aggregate->anomaly_count,
             (double)aggregate->sampling_frequency_hz,
             (double)aggregate->dominant_frequency_hz,
             (double)aggregate->average_value,
             message.created_at_us,
             message.publish_latency_us,
             lorawan_radio_tx_enabled() ? "radio_enabled" : "stub_ready");

    return message;
}

static void lorawan_queue_keep_latest(project_context_t *ctx, const transmission_payload_t *message)
{
    if (xQueueSend(ctx->lorawan_queue, message, 0) == pdPASS) {
        return;
    }

    // For cloud uplinks we care more about the newest aggregate than replaying stale ones,
    // so a full queue drops the oldest prepared message and keeps the latest result.
    transmission_payload_t discarded;
    if (xQueueReceive(ctx->lorawan_queue, &discarded, 0) == pdPASS) {
        (void)xQueueSend(ctx->lorawan_queue, message, 0);
        ESP_LOGW(TAG, "LoRaWAN queue full; discarded oldest prepared payload");
    }
}

esp_err_t comm_lorawan_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL || ctx->lorawan_queue == NULL ||
        ctx->aggregate_lorawan_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_LORAWAN_READY);
    if (!lorawan_path_enabled(ctx)) {
        ESP_LOGI(TAG, "LoRaWAN path disabled by communication mode");
    } else {
        ESP_LOGI(TAG,
                 "LoRaWAN module ready for TTN aggregate uplinks"
                 " (radio_tx=%u fport=%u topic=%s)",
                 (unsigned)lorawan_radio_tx_enabled(),
                 PROJECT_LORAWAN_FPORT,
                 PROJECT_LORAWAN_UPLINK_TOPIC);
    }
    return ESP_OK;
}

void comm_lorawan_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;
    aggregate_result_t aggregate;
    uint64_t last_status_us = (uint64_t)esp_timer_get_time();

    if (!lorawan_path_enabled(ctx)) {
        ESP_LOGI(TAG, "LoRaWAN task idle because LoRaWAN delivery is disabled");

        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(PROJECT_LORAWAN_HEARTBEAT_MS));
        }
    }

    for (;;) {
        if (xQueueReceive(ctx->aggregate_lorawan_queue, &aggregate, pdMS_TO_TICKS(250)) == pdPASS) {
            // This stage is "TTN-ready" even when radio TX is disabled: the payload format,
            // timing fields, and queue behavior are the same ones we will use on campus.
            const transmission_payload_t message = build_lorawan_message(&aggregate);

            lorawan_queue_keep_latest(ctx, &message);
            ctx->timing.lora_messages_prepared++;
            ESP_LOGI(TAG,
                     "Prepared LoRaWAN aggregate payload | window=%" PRIu32
                     " topic=%s payload=%s",
                     message.window_id,
                     message.topic,
                     message.payload);

            if (lorawan_radio_tx_enabled()) {
                ESP_LOGW(TAG,
                         "LoRaWAN radio transmission is enabled in config,"
                         " but the board-specific TTN driver is not integrated yet");
            }
        }

        const uint64_t now_us = (uint64_t)esp_timer_get_time();
        if ((now_us - last_status_us) >= (uint64_t)PROJECT_LORAWAN_HEARTBEAT_MS * 1000ULL) {
            ESP_LOGI(TAG,
                     "LoRaWAN heartbeat | radio_tx=%u pending=%u prepared=%" PRIu32,
                     (unsigned)lorawan_radio_tx_enabled(),
                     (unsigned)uxQueueMessagesWaiting(ctx->lorawan_queue),
                     ctx->timing.lora_messages_prepared);
            last_status_us = now_us;
        }
    }
}
