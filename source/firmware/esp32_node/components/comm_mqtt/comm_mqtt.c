#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

#include "comm_mqtt.h"
#include "project_config.h"
#include "project_types.h"

static const char *TAG = "comm_mqtt";

#define MQTT_WIFI_CONNECTED_BIT BIT0
#define MQTT_BROKER_CONNECTED_BIT BIT1

static EventGroupHandle_t s_comm_events;
static esp_mqtt_client_handle_t s_mqtt_client;
static uint32_t s_wifi_retry_count;
static uint8_t s_network_started;
static uint8_t s_mqtt_client_started;
static uint8_t s_sntp_started;
static uint8_t s_time_sync_ready;

static bool mqtt_tls_enabled(void)
{
    return PROJECT_MQTT_SECURITY_MODE == PROJECT_MQTT_SECURITY_TLS;
}

static const char *mqtt_scheme(void)
{
    return mqtt_tls_enabled() ? "mqtts" : "mqtt";
}

static bool mqtt_broker_credentials_configured(void)
{
    return PROJECT_MQTT_USERNAME[0] != '\0';
}

static bool mqtt_tls_common_name_configured(void)
{
    return PROJECT_MQTT_TLS_COMMON_NAME[0] != '\0';
}

static bool mqtt_path_enabled(const project_context_t *ctx)
{
    return ctx != NULL && project_comm_mode_supports_mqtt(ctx->communication_mode) != 0U;
}

static bool wifi_credentials_configured(void)
{
    return PROJECT_WIFI_SSID[0] != '\0';
}

static uint64_t unix_time_now_us(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000000ULL) + (uint64_t)tv.tv_usec;
}

static bool unix_time_offset_us(int64_t *out_offset_us)
{
    const uint64_t unix_now = unix_time_now_us();
    const uint64_t monotonic_now = (uint64_t)esp_timer_get_time();

    // Treat dates before 2024 as "not synced yet" because the default RTC time is meaningless
    // for cross-device latency measurements.
    if (unix_now < 1704067200000000ULL) {
        return false;
    }

    if (out_offset_us != NULL) {
        *out_offset_us = (int64_t)unix_now - (int64_t)monotonic_now;
    }
    return true;
}

static uint64_t monotonic_to_unix_us(uint64_t monotonic_us, int64_t offset_us)
{
    return (uint64_t)((int64_t)monotonic_us + offset_us);
}

static void sntp_sync_notification(struct timeval *tv)
{
    (void)tv;

    s_time_sync_ready = 1U;
    ESP_LOGI(TAG, "SNTP time synchronization completed");
}

static esp_err_t comm_mqtt_start_sntp(void)
{
    if (s_sntp_started != 0U) {
        return ESP_OK;
    }

    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG(PROJECT_DEFAULT_NTP_SERVER);
    sntp_config.sync_cb = sntp_sync_notification;

    esp_err_t err = esp_netif_sntp_init(&sntp_config);
    if (err != ESP_OK) {
        return err;
    }

    s_sntp_started = 1U;
    s_time_sync_ready = unix_time_offset_us(NULL) ? 1U : 0U;
    ESP_LOGI(TAG, "SNTP initialized with server %s", PROJECT_DEFAULT_NTP_SERVER);
    return ESP_OK;
}

static void mqtt_queue_enqueue(project_context_t *ctx, const transmission_payload_t *message)
{
    if (xQueueSend(ctx->mqtt_queue, message, pdMS_TO_TICKS(50)) != pdPASS) {
        ESP_LOGW(TAG,
                 "MQTT publish queue full; dropped window %" PRIu32,
                 message->window_id);
    }
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "WiFi station started; connecting to SSID '%s'", PROJECT_WIFI_SSID);
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupClearBits(s_comm_events, MQTT_WIFI_CONNECTED_BIT | MQTT_BROKER_CONNECTED_BIT);

            if (s_wifi_retry_count < PROJECT_WIFI_MAXIMUM_RETRY) {
                s_wifi_retry_count++;
                ESP_LOGW(TAG,
                         "WiFi disconnected; retry %" PRIu32 "/%u",
                         s_wifi_retry_count,
                         PROJECT_WIFI_MAXIMUM_RETRY);
                esp_wifi_connect();
            } else {
                ESP_LOGE(TAG,
                         "WiFi disconnected after %" PRIu32
                         " retries; waiting for next reconnect opportunity",
                         s_wifi_retry_count);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        s_wifi_retry_count = 0U;
        xEventGroupSetBits(s_comm_events, MQTT_WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG,
                 "WiFi connected | ip=" IPSTR,
                 IP2STR(&event->ip_info.ip));

        if (comm_mqtt_start_sntp() != ESP_OK) {
            ESP_LOGW(TAG, "SNTP initialization failed; MQTT timing stays monotonic-only");
        }
        s_time_sync_ready = unix_time_offset_us(NULL) ? 1U : s_time_sync_ready;

        if (s_mqtt_client != NULL) {
            if (s_mqtt_client_started == 0U) {
                ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
                s_mqtt_client_started = 1U;
                ESP_LOGI(TAG, "MQTT client started after WiFi obtained an IP address");
            } else {
                esp_err_t err = esp_mqtt_client_reconnect(s_mqtt_client);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Requested MQTT reconnect after WiFi recovery");
                } else {
                    ESP_LOGW(TAG, "MQTT reconnect request failed: %s", esp_err_to_name(err));
                }
            }
        }
    }
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(s_comm_events, MQTT_BROKER_CONNECTED_BIT);
            ESP_LOGI(TAG, "MQTT broker connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(s_comm_events, MQTT_BROKER_CONNECTED_BIT);
            ESP_LOGW(TAG, "MQTT broker disconnected");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG, "MQTT error event received");
            if (event != NULL && event->error_handle != NULL) {
                ESP_LOGW(TAG,
                         "MQTT error details | type=%d tls_esp_err=%s tls_stack_err=%d"
                         " cert_flags=%d sock_errno=%d connect_rc=%d",
                         (int)event->error_handle->error_type,
                         esp_err_to_name(event->error_handle->esp_tls_last_esp_err),
                         event->error_handle->esp_tls_stack_err,
                         event->error_handle->esp_tls_cert_verify_flags,
                         event->error_handle->esp_transport_sock_errno,
                         (int)event->error_handle->connect_return_code);
            }
            break;
        default:
            break;
    }
}

static esp_err_t comm_mqtt_start_network(project_context_t *ctx)
{
    if (s_network_started != 0U) {
        return ESP_OK;
    }

    // This bootstraps the whole edge path in one place: NVS, TCP/IP stack, WiFi, then MQTT.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    if (esp_netif_create_default_wifi_sta() == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi station netif");
        return ESP_FAIL;
    }

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = { 0 };
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", PROJECT_WIFI_SSID);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", PROJECT_WIFI_PASSWORD);
    wifi_config.sta.threshold.authmode =
        PROJECT_WIFI_PASSWORD[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());

    char mqtt_uri[128];
    snprintf(mqtt_uri,
             sizeof(mqtt_uri),
             "%s://%s:%d",
             mqtt_scheme(),
             PROJECT_DEFAULT_MQTT_BROKER_HOST,
             PROJECT_DEFAULT_MQTT_BROKER_PORT);

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = mqtt_uri,
        .credentials.client_id = PROJECT_DEVICE_NAME,
        .session.keepalive = 30,
        .network.disable_auto_reconnect = false,
    };

    if (mqtt_tls_enabled()) {
        // Use the built-in Espressif CA bundle so public CA-signed brokers can be
        // verified without embedding a custom PEM in the project.
        mqtt_config.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
        mqtt_config.broker.verification.skip_cert_common_name_check =
            PROJECT_MQTT_TLS_SKIP_COMMON_NAME_CHECK != 0U;
        if (mqtt_tls_common_name_configured()) {
            mqtt_config.broker.verification.common_name = PROJECT_MQTT_TLS_COMMON_NAME;
        }
    }

    if (mqtt_broker_credentials_configured()) {
        mqtt_config.credentials.username = PROJECT_MQTT_USERNAME;
        mqtt_config.credentials.authentication.password = PROJECT_MQTT_PASSWORD;
    }

    s_mqtt_client = esp_mqtt_client_init(&mqtt_config);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT client");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client,
                                                   ESP_EVENT_ANY_ID,
                                                   mqtt_event_handler,
                                                   ctx));

    s_network_started = 1U;
    return ESP_OK;
}

static transmission_payload_t build_mqtt_message(const aggregate_result_t *aggregate)
{
    // The payload carries both signal results and timing fields so the edge listener
    // can compute end-to-end latency without guessing when the board created the window.
    int64_t unix_offset_us = 0;
    const bool time_synced = unix_time_offset_us(&unix_offset_us);
    transmission_payload_t message = {
        .created_at_us = (uint64_t)esp_timer_get_time(),
        .window_id = aggregate->window_id,
        .source_timestamp_us = aggregate->window_end_us,
        .publish_latency_us = 0U,
    };

    if (message.created_at_us >= aggregate->window_end_us) {
        message.publish_latency_us = message.created_at_us - aggregate->window_end_us;
    }

    snprintf(message.topic, sizeof(message.topic), "%s", PROJECT_DEFAULT_MQTT_TOPIC);

    const int payload_length =
        snprintf(message.payload,
                 sizeof(message.payload),
                 "{\"device\":\"%s\",\"window_id\":%" PRIu32
                 ",\"window_start_us\":%" PRIu64 ",\"window_end_us\":%" PRIu64
                 ",\"published_at_us\":%" PRIu64 ",\"edge_delay_us\":%" PRIu64
                 ",\"window_start_unix_us\":%" PRIu64 ",\"window_end_unix_us\":%" PRIu64
                 ",\"published_at_unix_us\":%" PRIu64 ",\"time_sync_ready\":%s"
                 ",\"signal_profile\":\"%s\",\"anomaly_count\":%" PRIu32
                 ",\"sample_count\":%" PRIu32 ",\"sampling_frequency_hz\":%.1f"
                 ",\"dominant_frequency_hz\":%.2f,\"average_value\":%.4f}",
                 PROJECT_DEVICE_NAME,
                 aggregate->window_id,
                 aggregate->window_start_us,
                 aggregate->window_end_us,
                 message.created_at_us,
                 message.publish_latency_us,
                 time_synced ? monotonic_to_unix_us(aggregate->window_start_us, unix_offset_us) : 0ULL,
                 time_synced ? monotonic_to_unix_us(aggregate->window_end_us, unix_offset_us) : 0ULL,
                 time_synced ? monotonic_to_unix_us(message.created_at_us, unix_offset_us) : 0ULL,
                 time_synced ? "true" : "false",
                 project_signal_profile_name(aggregate->signal_profile),
                 aggregate->anomaly_count,
                 aggregate->sample_count,
                 (double)aggregate->sampling_frequency_hz,
                 (double)aggregate->dominant_frequency_hz,
                 (double)aggregate->average_value);

    if (payload_length < 0 || payload_length >= (int)sizeof(message.payload)) {
        ESP_LOGW(TAG,
                 "MQTT payload truncated for window %" PRIu32 " (needed=%d bytes, capacity=%u)",
                 aggregate->window_id,
                 payload_length,
                 (unsigned)sizeof(message.payload));
    }

    return message;
}

static void publish_pending_messages(project_context_t *ctx)
{
    if ((xEventGroupGetBits(s_comm_events) & MQTT_BROKER_CONNECTED_BIT) == 0U || s_mqtt_client == NULL) {
        return;
    }

    // We only remove a message from the queue after the client accepts it for publish,
    // which keeps temporary broker outages from silently dropping aggregates.
    transmission_payload_t message;
    while (xQueuePeek(ctx->mqtt_queue, &message, 0) == pdPASS) {
        const int message_id = esp_mqtt_client_publish(s_mqtt_client,
                                                       message.topic,
                                                       message.payload,
                                                       0,
                                                       1,
                                                       0);
        if (message_id < 0) {
            ESP_LOGW(TAG,
                     "MQTT publish failed for window %" PRIu32 "; will retry",
                     message.window_id);
            break;
        }

        (void)xQueueReceive(ctx->mqtt_queue, &message, 0);
        ctx->timing.mqtt_messages_sent++;
        ctx->timing.last_publish_us = (uint64_t)esp_timer_get_time();
        ctx->timing.last_edge_latency_us =
            ctx->timing.last_publish_us >= message.source_timestamp_us
                ? ctx->timing.last_publish_us - message.source_timestamp_us
                : 0U;

        ESP_LOGI(TAG,
                 "Published MQTT aggregate | window=%" PRIu32 " msg_id=%d topic=%s"
                 " edge_latency_us=%" PRIu64,
                 message.window_id,
                 message_id,
                 message.topic,
                 ctx->timing.last_edge_latency_us);
    }
}

esp_err_t comm_mqtt_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL || ctx->mqtt_queue == NULL ||
        ctx->aggregate_mqtt_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!mqtt_path_enabled(ctx)) {
        xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_MQTT_READY);
        ESP_LOGI(TAG, "MQTT path disabled by communication mode");
        return ESP_OK;
    }

    s_comm_events = xEventGroupCreate();
    if (s_comm_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    if (!wifi_credentials_configured()) {
        ESP_LOGW(TAG,
                 "WiFi SSID is empty; set PROJECT_WIFI_SSID and PROJECT_WIFI_PASSWORD"
                 " in project_config_local.h before Phase 8 runtime testing");
    } else {
        esp_err_t err = comm_mqtt_start_network(ctx);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start WiFi/MQTT network stack: %s", esp_err_to_name(err));
            return err;
        }
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_MQTT_READY);
    ESP_LOGI(TAG,
             "MQTT module ready for broker %s:%d on topic %s"
             " | transport=%s auth=%u",
             PROJECT_DEFAULT_MQTT_BROKER_HOST,
             PROJECT_DEFAULT_MQTT_BROKER_PORT,
             PROJECT_DEFAULT_MQTT_TOPIC,
             mqtt_scheme(),
             (unsigned)mqtt_broker_credentials_configured());
    return ESP_OK;
}

void comm_mqtt_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;
    aggregate_result_t aggregate;
    uint64_t last_status_us = (uint64_t)esp_timer_get_time();

    if (!mqtt_path_enabled(ctx)) {
        ESP_LOGI(TAG, "MQTT task idle because MQTT delivery is disabled");

        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(PROJECT_MQTT_HEARTBEAT_MS));
        }
    }

    for (;;) {
        if (xQueueReceive(ctx->aggregate_mqtt_queue, &aggregate, pdMS_TO_TICKS(250)) == pdPASS) {
            if (!wifi_credentials_configured()) {
                ESP_LOGW(TAG,
                         "Skipping MQTT publish for window %" PRIu32
                         " because WiFi credentials are not configured",
                         aggregate.window_id);
            } else {
                // Aggregate -> MQTT payload conversion happens here so the processing task
                // stays focused on signal work and does not know transport details.
                const transmission_payload_t message = build_mqtt_message(&aggregate);
                mqtt_queue_enqueue(ctx, &message);
                ctx->timing.mqtt_messages_prepared++;
                ESP_LOGI(TAG,
                         "Prepared MQTT aggregate payload | window=%" PRIu32
                         " topic=%s profile=%s anomalies=%" PRIu32
                         " edge_delay_us=%" PRIu64 " time_sync=%u",
                         message.window_id,
                         message.topic,
                         project_signal_profile_name(aggregate.signal_profile),
                         aggregate.anomaly_count,
                         message.publish_latency_us,
                         (unsigned)s_time_sync_ready);
            }
        }

        publish_pending_messages(ctx);

        const uint64_t now_us = (uint64_t)esp_timer_get_time();
        if ((now_us - last_status_us) >= (uint64_t)PROJECT_MQTT_HEARTBEAT_MS * 1000ULL) {
            const EventBits_t bits = xEventGroupGetBits(s_comm_events);
            ESP_LOGI(TAG,
                     "MQTT heartbeat | wifi=%u mqtt=%u pending=%u prepared=%" PRIu32
                     " sent=%" PRIu32,
                     (unsigned)((bits & MQTT_WIFI_CONNECTED_BIT) != 0U),
                     (unsigned)((bits & MQTT_BROKER_CONNECTED_BIT) != 0U),
                     (unsigned)uxQueueMessagesWaiting(ctx->mqtt_queue),
                     ctx->timing.mqtt_messages_prepared,
                     ctx->timing.mqtt_messages_sent);
            last_status_us = now_us;
        }
    }
}
