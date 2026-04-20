#include <cinttypes>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Arduino.h"
#include "LoRaWan_APP.h"
#include "radio/radio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "comm_lorawan.h"
#include "project_config.h"
#include "project_types.h"

static const char *TAG = "comm_lorawan";

namespace {

constexpr size_t kLorawanPayloadSize = 10U;
constexpr size_t kJoinEuiSize = 8U;
constexpr size_t kDevEuiSize = 8U;
constexpr size_t kAppKeySize = 16U;
constexpr TickType_t kLorawanPollDelay = pdMS_TO_TICKS(25);

bool s_runtime_initialized = false;
bool s_join_state_logged = false;
bool s_credentials_error_logged = false;

uint32_t s_heltec_license_words[4] = PROJECT_LORAWAN_HELTEC_LICENSE_WORDS;

bool lorawan_path_enabled(const project_context_t *ctx)
{
    return ctx != nullptr && project_comm_mode_supports_lorawan(ctx->communication_mode) != 0U;
}

bool lorawan_radio_tx_enabled(void)
{
    return PROJECT_LORAWAN_ENABLE_RADIO_TX != 0U;
}

bool has_nonzero_bytes(const uint8_t *bytes, size_t byte_count)
{
    if (bytes == nullptr) {
        return false;
    }

    for (size_t index = 0; index < byte_count; ++index) {
        if (bytes[index] != 0U) {
            return true;
        }
    }

    return false;
}

bool has_nonzero_words(const uint32_t *words, size_t word_count)
{
    if (words == nullptr) {
        return false;
    }

    for (size_t index = 0; index < word_count; ++index) {
        if (words[index] != 0U) {
            return true;
        }
    }

    return false;
}

bool lorawan_runtime_configured(void)
{
    if (!has_nonzero_bytes(appKey, kAppKeySize)) {
        return false;
    }

    if (PROJECT_LORAWAN_USE_CHIP_GENERATED_DEV_EUI != 0U) {
        return true;
    }

    return has_nonzero_bytes(devEui, kDevEuiSize);
}

bool lorawan_network_joined(void)
{
    MibRequestConfirm_t mib_req = {};
    mib_req.Type = MIB_NETWORK_JOINED;

    return LoRaMacMibGetRequestConfirm(&mib_req) == LORAMAC_STATUS_OK &&
           mib_req.Param.IsNetworkJoined == true;
}

uint16_t clamp_u16_from_float(float value, float scale)
{
    const float scaled_value = value * scale;

    if (scaled_value <= 0.0f) {
        return 0U;
    }
    if (scaled_value >= 65535.0f) {
        return 65535U;
    }
    return static_cast<uint16_t>(scaled_value + 0.5f);
}

int16_t clamp_i16_from_float(float value, float scale)
{
    const float scaled_value = value * scale;

    if (scaled_value <= static_cast<float>(INT16_MIN)) {
        return INT16_MIN;
    }
    if (scaled_value >= static_cast<float>(INT16_MAX)) {
        return INT16_MAX;
    }

    if (scaled_value >= 0.0f) {
        return static_cast<int16_t>(scaled_value + 0.5f);
    }

    return static_cast<int16_t>(scaled_value - 0.5f);
}

void encode_u16_be(uint8_t *buffer, uint16_t value)
{
    buffer[0] = static_cast<uint8_t>((value >> 8) & 0xFFU);
    buffer[1] = static_cast<uint8_t>(value & 0xFFU);
}

void encode_i16_be(uint8_t *buffer, int16_t value)
{
    encode_u16_be(buffer, static_cast<uint16_t>(value));
}

void bytes_to_hex(const uint8_t *bytes, size_t byte_count, char *out_hex, size_t out_hex_size)
{
    static const char s_hex_digits[] = "0123456789ABCDEF";
    const size_t required_size = (byte_count * 2U) + 1U;

    if (out_hex == nullptr || out_hex_size < required_size) {
        return;
    }

    for (size_t index = 0; index < byte_count; ++index) {
        out_hex[index * 2U] = s_hex_digits[(bytes[index] >> 4) & 0x0FU];
        out_hex[(index * 2U) + 1U] = s_hex_digits[bytes[index] & 0x0FU];
    }

    out_hex[required_size - 1U] = '\0';
}

transmission_payload_t build_lorawan_message(const aggregate_result_t *aggregate)
{
    char payload_hex[(kLorawanPayloadSize * 2U) + 1U];
    transmission_payload_t message = {
        .created_at_us = static_cast<uint64_t>(esp_timer_get_time()),
        .window_id = aggregate->window_id,
        .source_timestamp_us = aggregate->window_end_us,
        .publish_latency_us = 0U,
        .port = static_cast<uint8_t>(PROJECT_LORAWAN_FPORT),
        .binary_payload_size = static_cast<uint8_t>(kLorawanPayloadSize),
        .binary_payload = { 0U },
        .topic = { 0 },
        .payload = { 0 },
    };

    if (message.created_at_us >= aggregate->window_end_us) {
        message.publish_latency_us = message.created_at_us - aggregate->window_end_us;
    }

    encode_u16_be(&message.binary_payload[0], static_cast<uint16_t>(aggregate->window_id & 0xFFFFU));
    encode_u16_be(&message.binary_payload[2],
                  static_cast<uint16_t>(aggregate->sample_count & 0xFFFFU));
    encode_u16_be(&message.binary_payload[4],
                  clamp_u16_from_float(aggregate->sampling_frequency_hz, 10.0f));
    encode_u16_be(&message.binary_payload[6],
                  clamp_u16_from_float(aggregate->dominant_frequency_hz, 100.0f));
    encode_i16_be(&message.binary_payload[8], clamp_i16_from_float(aggregate->average_value, 1000.0f));
    bytes_to_hex(message.binary_payload,
                 message.binary_payload_size,
                 payload_hex,
                 sizeof(payload_hex));

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
             static_cast<double>(aggregate->sampling_frequency_hz),
             static_cast<double>(aggregate->dominant_frequency_hz),
             static_cast<double>(aggregate->average_value),
             message.created_at_us,
             message.publish_latency_us,
             lorawan_radio_tx_enabled() ? "queued_for_radio" : "prepared_only");

    return message;
}

void lorawan_queue_keep_latest(project_context_t *ctx, const transmission_payload_t *message)
{
    if (xQueueSend(ctx->lorawan_queue, message, 0) == pdPASS) {
        return;
    }

    transmission_payload_t discarded;
    if (xQueueReceive(ctx->lorawan_queue, &discarded, 0) == pdPASS) {
        (void)xQueueSend(ctx->lorawan_queue, message, 0);
        ESP_LOGW(TAG, "LoRaWAN queue full; discarded oldest prepared payload");
    }
}

void lorawan_apply_license_if_available(void)
{
    if (PROJECT_LORAWAN_APPLY_HELTEC_LICENSE_AT_BOOT == 0U) {
        ESP_LOGI(TAG, "Using stored Heltec board activation; runtime license apply disabled");
        return;
    }

    if (!has_nonzero_words(s_heltec_license_words, sizeof(s_heltec_license_words) / sizeof(s_heltec_license_words[0]))) {
        ESP_LOGI(TAG, "Heltec license words not provided in local config; using stored board activation");
        return;
    }

    Mcu.setlicense(s_heltec_license_words, HELTEC_BOARD);
    delay(50);
    ESP_LOGI(TAG, "Applied Heltec board license from local config");
}

bool lorawan_bootstrap_runtime(void)
{
    if (s_runtime_initialized) {
        return true;
    }

    initArduino();
    Serial.begin(115200);
    delay(250);

    lorawan_apply_license_if_available();
    const int board_init_result = Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    if (PROJECT_LORAWAN_USE_CHIP_GENERATED_DEV_EUI != 0U) {
        LoRaWAN.generateDeveuiByChipID();
    }

    s_runtime_initialized = true;
    ESP_LOGI(TAG,
             "Heltec LoRaWAN runtime bootstrapped | board_init=%d adr=%u confirmed=%u duty_ms=%" PRIu32,
             board_init_result,
             static_cast<unsigned>(PROJECT_LORAWAN_ADR_ENABLED),
             static_cast<unsigned>(PROJECT_LORAWAN_CONFIRMED_UPLINKS),
             static_cast<uint32_t>(PROJECT_LORAWAN_TX_DUTY_CYCLE_MS));
    return true;
}

void lorawan_service_background(void)
{
    if (!s_runtime_initialized) {
        return;
    }

    Mcu.timerhandler();
    Radio.IrqProcess();
}

void lorawan_log_join_state_if_needed(void)
{
    const bool joined = lorawan_network_joined();

    if (joined && !s_join_state_logged) {
        char dev_eui_hex[(kDevEuiSize * 2U) + 1U];
        bytes_to_hex(devEui, kDevEuiSize, dev_eui_hex, sizeof(dev_eui_hex));
        ESP_LOGI(TAG, "LoRaWAN joined TTN successfully | dev_eui=%s fport=%u", dev_eui_hex, appPort);
        s_join_state_logged = true;
        return;
    }

    if (!joined && s_join_state_logged) {
        ESP_LOGW(TAG, "LoRaWAN network join lost; waiting for rejoin");
        s_join_state_logged = false;
    }
}

void lorawan_mark_send_ready_if_possible(project_context_t *ctx)
{
    if (!s_runtime_initialized || !lorawan_network_joined()) {
        return;
    }

    if (uxQueueMessagesWaiting(ctx->lorawan_queue) == 0U) {
        return;
    }

    if (deviceState == DEVICE_STATE_SLEEP) {
        deviceState = DEVICE_STATE_SEND;
    }
}

bool lorawan_submit_next_message(project_context_t *ctx)
{
    transmission_payload_t message;
    MibRequestConfirm_t mib_req = {};

    if (xQueuePeek(ctx->lorawan_queue, &message, 0) != pdPASS) {
        return false;
    }

    mib_req.Type = MIB_DEVICE_CLASS;
    if (LoRaMacMibGetRequestConfirm(&mib_req) == LORAMAC_STATUS_OK &&
        loraWanClass != mib_req.Param.Class) {
        mib_req.Param.Class = loraWanClass;
        (void)LoRaMacMibSetRequestConfirm(&mib_req);
    }

    memcpy(appData, message.binary_payload, message.binary_payload_size);
    appDataSize = message.binary_payload_size;
    appPort = message.port;

    if (SendFrame()) {
        ESP_LOGW(TAG,
                 "LoRaWAN MAC not ready yet; keeping payload queued | window=%" PRIu32,
                 message.window_id);
        return false;
    }

    transmission_payload_t committed_message;
    (void)xQueueReceive(ctx->lorawan_queue, &committed_message, 0);
    ctx->timing.lora_messages_sent++;
    ctx->timing.last_publish_us = static_cast<uint64_t>(esp_timer_get_time());
    ESP_LOGI(TAG,
             "LoRaWAN uplink queued to radio | window=%" PRIu32 " bytes=%u pending=%u",
             message.window_id,
             static_cast<unsigned>(message.binary_payload_size),
             static_cast<unsigned>(uxQueueMessagesWaiting(ctx->lorawan_queue)));
    return true;
}

void lorawan_step_state_machine(project_context_t *ctx)
{
    switch (deviceState) {
        case DEVICE_STATE_INIT:
            ESP_LOGI(TAG, "Initializing Heltec LoRaWAN stack for main app");
            LoRaWAN.init(loraWanClass, loraWanRegion);
            LoRaWAN.setDefaultDR(PROJECT_LORAWAN_DEFAULT_DR);
            break;

        case DEVICE_STATE_JOIN:
            ESP_LOGI(TAG, "Sending OTAA join request to TTN");
            LoRaWAN.join();
            break;

        case DEVICE_STATE_SEND:
            if (!lorawan_network_joined()) {
                deviceState = DEVICE_STATE_SLEEP;
                break;
            }

            if (lorawan_submit_next_message(ctx)) {
                deviceState = DEVICE_STATE_CYCLE;
            } else {
                deviceState = DEVICE_STATE_SLEEP;
            }
            break;

        case DEVICE_STATE_CYCLE:
            LoRaWAN.cycle(appTxDutyCycle);
            deviceState = DEVICE_STATE_SLEEP;
            break;

        case DEVICE_STATE_SLEEP:
        default:
            break;
    }
}

}  // namespace

uint8_t devEui[] = PROJECT_LORAWAN_DEV_EUI_BYTES;
uint8_t appEui[] = PROJECT_LORAWAN_JOIN_EUI_BYTES;
uint8_t appKey[] = PROJECT_LORAWAN_APP_KEY_BYTES;
uint8_t nwkSKey[] = { 0U };
uint8_t appSKey[] = { 0U };
uint32_t devAddr = 0U;
uint16_t userChannelsMask[6] = PROJECT_LORAWAN_CHANNEL_MASK;
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = PROJECT_LORAWAN_TX_DUTY_CYCLE_MS;
bool overTheAirActivation = true;
bool loraWanAdr = PROJECT_LORAWAN_ADR_ENABLED != 0U;
bool isTxConfirmed = PROJECT_LORAWAN_CONFIRMED_UPLINKS != 0U;
uint8_t appPort = static_cast<uint8_t>(PROJECT_LORAWAN_FPORT);
uint8_t confirmedNbTrials = PROJECT_LORAWAN_CONFIRMED_TRIALS;
bool keepNet = false;

extern "C" esp_err_t comm_lorawan_init(project_context_t *ctx)
{
    if (ctx == nullptr || ctx->system_events == nullptr || ctx->lorawan_queue == nullptr ||
        ctx->aggregate_lorawan_queue == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_LORAWAN_READY);
    if (!lorawan_path_enabled(ctx)) {
        ESP_LOGI(TAG, "LoRaWAN path disabled by communication mode");
        return ESP_OK;
    }

    if (!lorawan_radio_tx_enabled()) {
        ESP_LOGI(TAG,
                 "LoRaWAN payload path ready in preparation-only mode"
                 " (radio_tx=%u fport=%u topic=%s)",
                 static_cast<unsigned>(lorawan_radio_tx_enabled()),
                 PROJECT_LORAWAN_FPORT,
                 PROJECT_LORAWAN_UPLINK_TOPIC);
        return ESP_OK;
    }

    if (!lorawan_runtime_configured()) {
        ESP_LOGW(TAG,
                 "LoRaWAN radio path enabled, but credentials are incomplete."
                 " Populate project_config_local.h before expecting TTN uplinks.");
    }

    ESP_LOGI(TAG,
             "LoRaWAN module ready with integrated Heltec radio backend"
             " (radio_tx=%u fport=%u topic=%s)",
             static_cast<unsigned>(lorawan_radio_tx_enabled()),
             PROJECT_LORAWAN_FPORT,
             PROJECT_LORAWAN_UPLINK_TOPIC);
    return ESP_OK;
}

extern "C" void comm_lorawan_task(void *pvParameters)
{
    auto *ctx = static_cast<project_context_t *>(pvParameters);
    aggregate_result_t aggregate;
    uint64_t last_status_us = static_cast<uint64_t>(esp_timer_get_time());

    if (!lorawan_path_enabled(ctx)) {
        ESP_LOGI(TAG, "LoRaWAN task idle because LoRaWAN delivery is disabled");

        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(PROJECT_LORAWAN_HEARTBEAT_MS));
        }
    }

    for (;;) {
        if (xQueueReceive(ctx->aggregate_lorawan_queue, &aggregate, kLorawanPollDelay) == pdPASS) {
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
                lorawan_mark_send_ready_if_possible(ctx);
            }
        }

        if (lorawan_radio_tx_enabled()) {
            if (!lorawan_runtime_configured()) {
                if (!s_credentials_error_logged) {
                    ESP_LOGE(TAG,
                             "LoRaWAN runtime cannot start until DevEUI/AppKey are set in "
                             "project_config_local.h");
                    s_credentials_error_logged = true;
                }
            } else if (lorawan_bootstrap_runtime()) {
                lorawan_step_state_machine(ctx);
                lorawan_service_background();
                lorawan_log_join_state_if_needed();
                lorawan_mark_send_ready_if_possible(ctx);
            }
        }

        const uint64_t now_us = static_cast<uint64_t>(esp_timer_get_time());
        if ((now_us - last_status_us) >= static_cast<uint64_t>(PROJECT_LORAWAN_HEARTBEAT_MS) * 1000ULL) {
            ESP_LOGI(TAG,
                     "LoRaWAN heartbeat | radio_tx=%u joined=%u pending=%u prepared=%" PRIu32
                     " sent=%" PRIu32,
                     static_cast<unsigned>(lorawan_radio_tx_enabled()),
                     static_cast<unsigned>(lorawan_radio_tx_enabled() && lorawan_network_joined()),
                     static_cast<unsigned>(uxQueueMessagesWaiting(ctx->lorawan_queue)),
                     ctx->timing.lora_messages_prepared,
                     ctx->timing.lora_messages_sent);
            last_status_us = now_us;
        }
    }
}
