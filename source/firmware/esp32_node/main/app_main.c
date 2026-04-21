#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "app_display.h"
#include "comm_lorawan.h"
#include "comm_mqtt.h"
#include "metrics.h"
#include "project_config.h"
#include "project_types.h"
#include "sampling_control.h"
#include "signal_input.h"
#include "signal_processing.h"

static const char *TAG = "app_main";
static project_context_t g_project_context;

static void configure_serial_output_mode(void)
{
    if (PROJECT_ENABLE_BETTER_SERIAL_PLOTTER == 0U) {
        return;
    }

    if (PROJECT_BETTER_SERIAL_PLOTTER_SILENCE_INFO_LOGS != 0U) {
        // BetterSerialPlotter expects clean numeric rows. Silencing the normal INFO logs
        // keeps the serial stream easy to graph without removing warning/error visibility.
        esp_log_level_set("*", ESP_LOG_WARN);
    }
}

static project_comm_mode_t project_comm_mode_from_config(void)
{
    switch (PROJECT_COMMUNICATION_MODE) {
        case PROJECT_COMMUNICATION_MODE_MQTT_ONLY:
            return PROJECT_COMM_MODE_MQTT_ONLY;
        case PROJECT_COMMUNICATION_MODE_LORAWAN_ONLY:
            return PROJECT_COMM_MODE_LORAWAN_ONLY;
        case PROJECT_COMMUNICATION_MODE_BOTH:
        default:
            return PROJECT_COMM_MODE_BOTH;
    }
}

static signal_profile_t project_signal_profile_from_config(void)
{
    switch (PROJECT_SIGNAL_PROFILE) {
        case PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE:
            return SIGNAL_PROFILE_CLEAN_REFERENCE;
        case PROJECT_SIGNAL_PROFILE_NOISY_REFERENCE:
            return SIGNAL_PROFILE_NOISY_REFERENCE;
        case PROJECT_SIGNAL_PROFILE_ANOMALY_STRESS:
            return SIGNAL_PROFILE_ANOMALY_STRESS;
        default:
            return SIGNAL_PROFILE_CLEAN_REFERENCE;
    }
}

static const char *project_mode_to_string(project_mode_t mode)
{
    switch (mode) {
        case PROJECT_MODE_PHASE2_SKELETON:
            return "phase2_skeleton";
        case PROJECT_MODE_VIRTUAL_SENSOR:
            return "virtual_sensor";
        case PROJECT_MODE_HC_SR04_DEMO:
            return "hc_sr04_demo";
        case PROJECT_MODE_SAMPLING_BENCHMARK:
            return "sampling_benchmark";
        case PROJECT_MODE_RAW_SAMPLING_BENCHMARK:
            return "raw_sampling_benchmark";
        default:
            return "unknown";
    }
}

static bool project_mode_uses_minimal_benchmark_tasks(project_mode_t mode)
{
    return mode == PROJECT_MODE_SAMPLING_BENCHMARK ||
           mode == PROJECT_MODE_RAW_SAMPLING_BENCHMARK;
}

static bool project_deep_sleep_experiment_enabled(const project_context_t *ctx)
{
    return ctx != NULL &&
           PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT != 0U &&
           !project_mode_uses_minimal_benchmark_tasks(ctx->mode);
}

static bool project_mqtt_publish_required_before_sleep(const project_context_t *ctx)
{
    return PROJECT_DEEP_SLEEP_WAIT_FOR_MQTT_PUBLISH != 0U &&
           ctx != NULL &&
           project_comm_mode_supports_mqtt(ctx->communication_mode) != 0U &&
           PROJECT_WIFI_SSID[0] != '\0';
}

static bool project_deep_sleep_ready(const project_context_t *ctx,
                                     const timing_metrics_t *snapshot,
                                     uint64_t now_us)
{
    if (!project_deep_sleep_experiment_enabled(ctx) || snapshot == NULL) {
        return false;
    }

    const uint32_t target_aggregates = PROJECT_DEEP_SLEEP_AFTER_AGGREGATES == 0U
                                           ? 1U
                                           : PROJECT_DEEP_SLEEP_AFTER_AGGREGATES;
    if (snapshot->aggregates_processed < target_aggregates) {
        return false;
    }

    const bool mqtt_publish_required = project_mqtt_publish_required_before_sleep(ctx);
    if (!mqtt_publish_required || snapshot->mqtt_messages_sent >= target_aggregates) {
        return true;
    }

    const uint64_t awake_us = now_us >= ctx->timing.boot_time_us
                                  ? now_us - ctx->timing.boot_time_us
                                  : 0ULL;
    const uint64_t max_awake_us = (uint64_t)PROJECT_DEEP_SLEEP_MAX_AWAKE_SEC * 1000000ULL;
    if (PROJECT_DEEP_SLEEP_MAX_AWAKE_SEC != 0U && awake_us >= max_awake_us) {
        ESP_LOGW(TAG,
                 "Deep-sleep experiment timeout before MQTT publish | aggregates=%" PRIu32
                 " mqtt_sent=%" PRIu32 " max_awake_s=%u",
                 snapshot->aggregates_processed,
                 snapshot->mqtt_messages_sent,
                 PROJECT_DEEP_SLEEP_MAX_AWAKE_SEC);
        return true;
    }

    return false;
}

static void project_enter_deep_sleep(const project_context_t *ctx,
                                     const timing_metrics_t *snapshot)
{
    const uint64_t sleep_us = (uint64_t)PROJECT_DEEP_SLEEP_DURATION_SEC * 1000000ULL;

    ESP_LOGW(TAG,
             "Entering deep-sleep experiment | sleep_s=%u aggregates=%" PRIu32
             " mqtt_sent=%" PRIu32 " fs=%.1fHz dominant=%.2fHz",
             PROJECT_DEEP_SLEEP_DURATION_SEC,
             snapshot != NULL ? snapshot->aggregates_processed : 0U,
             snapshot != NULL ? snapshot->mqtt_messages_sent : 0U,
             ctx != NULL ? (double)ctx->adaptive.current_sampling_frequency_hz : 0.0,
             ctx != NULL ? (double)ctx->adaptive.last_dominant_frequency_hz : 0.0);

    (void)esp_sleep_enable_timer_wakeup(sleep_us);
    (void)esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(PROJECT_DEEP_SLEEP_GRACE_MS));
    esp_deep_sleep_start();
}

static const char *project_comm_mode_to_string(project_comm_mode_t mode)
{
    switch (mode) {
        case PROJECT_COMM_MODE_MQTT_ONLY:
            return "mqtt_only";
        case PROJECT_COMM_MODE_LORAWAN_ONLY:
            return "lorawan_only";
        case PROJECT_COMM_MODE_BOTH:
            return "both";
        default:
            return "unknown";
    }
}

static bool project_context_create(project_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    // One shared context keeps every task looking at the same queues, counters, and mode flags.
    ctx->sample_queue = xQueueCreate(PROJECT_SAMPLE_QUEUE_LENGTH, sizeof(raw_sample_t));
    ctx->fft_queue = xQueueCreate(PROJECT_FFT_QUEUE_LENGTH, sizeof(fft_result_t));
    ctx->aggregate_mqtt_queue =
        xQueueCreate(PROJECT_AGGREGATE_QUEUE_LENGTH, sizeof(aggregate_result_t));
    ctx->aggregate_lorawan_queue =
        xQueueCreate(PROJECT_AGGREGATE_QUEUE_LENGTH, sizeof(aggregate_result_t));
    ctx->mqtt_queue = xQueueCreate(PROJECT_MQTT_QUEUE_LENGTH, sizeof(transmission_payload_t));
    ctx->lorawan_queue = xQueueCreate(PROJECT_LORAWAN_QUEUE_LENGTH, sizeof(transmission_payload_t));
    ctx->system_events = xEventGroupCreate();
    ctx->mode = PROJECT_ENABLE_RAW_SAMPLING_BENCHMARK
                    ? PROJECT_MODE_RAW_SAMPLING_BENCHMARK
                    : (PROJECT_ENABLE_SAMPLING_BENCHMARK ? PROJECT_MODE_SAMPLING_BENCHMARK
                                                         : PROJECT_MODE_VIRTUAL_SENSOR);
    ctx->communication_mode = project_comm_mode_from_config();
    ctx->signal_profile = project_signal_profile_from_config();
    ctx->timing.boot_time_us = (uint64_t)esp_timer_get_time();
    ctx->adaptive.current_sampling_frequency_hz = PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ;
    ctx->adaptive.last_requested_sampling_frequency_hz = PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ;
    ctx->adaptive.last_update_us = ctx->timing.boot_time_us;

    return ctx->sample_queue != NULL &&
           ctx->fft_queue != NULL &&
           ctx->aggregate_mqtt_queue != NULL &&
           ctx->aggregate_lorawan_queue != NULL &&
           ctx->mqtt_queue != NULL &&
           ctx->lorawan_queue != NULL &&
           ctx->system_events != NULL;
}

static void log_queue_status(const project_context_t *ctx)
{
    ESP_LOGI(TAG,
             "Queue status | sample=%u fft=%u aggregate_mqtt=%u aggregate_lora=%u"
             " mqtt=%u lora=%u",
             (unsigned)uxQueueMessagesWaiting(ctx->sample_queue),
             (unsigned)uxQueueMessagesWaiting(ctx->fft_queue),
             (unsigned)uxQueueMessagesWaiting(ctx->aggregate_mqtt_queue),
             (unsigned)uxQueueMessagesWaiting(ctx->aggregate_lorawan_queue),
             (unsigned)uxQueueMessagesWaiting(ctx->mqtt_queue),
             (unsigned)uxQueueMessagesWaiting(ctx->lorawan_queue));
}

void app_main(void)
{
    configure_serial_output_mode();

    if (!project_context_create(&g_project_context)) {
        ESP_LOGE(TAG, "Failed to allocate queues or event groups for the project context");
        return;
    }

    ESP_LOGI(TAG,
             "Firmware booting for %s in %s mode",
             PROJECT_DEVICE_NAME,
             project_mode_to_string(g_project_context.mode));
    ESP_LOGI(TAG,
             "Communication mode: %s",
             project_comm_mode_to_string(g_project_context.communication_mode));
    ESP_LOGI(TAG,
             "Signal profile: %s",
             project_signal_profile_name(g_project_context.signal_profile));
    ESP_LOGI(TAG,
             "Planned MQTT endpoint: %s:%d",
             PROJECT_DEFAULT_MQTT_BROKER_HOST,
             PROJECT_DEFAULT_MQTT_BROKER_PORT);

    ESP_ERROR_CHECK(metrics_init(&g_project_context));
    ESP_ERROR_CHECK(signal_input_init(&g_project_context));
    if (g_project_context.mode != PROJECT_MODE_RAW_SAMPLING_BENCHMARK) {
        ESP_ERROR_CHECK(signal_processing_init(&g_project_context));
    }

    if (!project_mode_uses_minimal_benchmark_tasks(g_project_context.mode)) {
        ESP_ERROR_CHECK(sampling_control_init(&g_project_context));
        ESP_ERROR_CHECK(comm_mqtt_init(&g_project_context));
        ESP_ERROR_CHECK(comm_lorawan_init(&g_project_context));
        ESP_ERROR_CHECK(app_display_init(&g_project_context));
    }

    xEventGroupSetBits(g_project_context.system_events, PROJECT_EVENT_BOOT_COMPLETE);

    // Each task owns one stage of the pipeline, so boot is mostly just wiring the stages together.
    xTaskCreate(signal_input_task,
                "signal_input_task",
                PROJECT_TASK_STACK_SIZE,
                &g_project_context,
                PROJECT_TASK_PRIORITY_INPUT,
                NULL);

    if (g_project_context.mode != PROJECT_MODE_RAW_SAMPLING_BENCHMARK) {
        xTaskCreate(signal_processing_task,
                    "signal_processing_task",
                    PROJECT_TASK_STACK_SIZE,
                    &g_project_context,
                    PROJECT_TASK_PRIORITY_PROCESSING,
                    NULL);
    }

    if (!project_mode_uses_minimal_benchmark_tasks(g_project_context.mode)) {
        xTaskCreate(sampling_control_task,
                    "sampling_control_task",
                    PROJECT_TASK_STACK_SIZE,
                    &g_project_context,
                    PROJECT_TASK_PRIORITY_CONTROL,
                    NULL);

        xTaskCreate(comm_mqtt_task,
                    "comm_mqtt_task",
                    PROJECT_TASK_STACK_SIZE,
                    &g_project_context,
                    PROJECT_TASK_PRIORITY_COMM,
                    NULL);

        xTaskCreate(comm_lorawan_task,
                    "comm_lorawan_task",
                    PROJECT_LORAWAN_TASK_STACK_SIZE,
                    &g_project_context,
                    PROJECT_TASK_PRIORITY_COMM,
                    NULL);
    }

    xTaskCreate(metrics_task,
                "metrics_task",
                PROJECT_TASK_STACK_SIZE,
                &g_project_context,
                PROJECT_TASK_PRIORITY_METRICS,
                NULL);

    if (!project_mode_uses_minimal_benchmark_tasks(g_project_context.mode)) {
        xTaskCreate(app_display_task,
                    "app_display_task",
                    PROJECT_DISPLAY_TASK_STACK_SIZE,
                    &g_project_context,
                    PROJECT_TASK_PRIORITY_DISPLAY,
                    NULL);
    }

    ESP_LOGI(TAG, "Application tasks created successfully");

    while (true) {
        // The supervisor does not process data itself; it just gives us a single place
        // to watch the whole pipeline stay healthy while the worker tasks run.
        EventBits_t bits = xEventGroupGetBits(g_project_context.system_events);
        timing_metrics_t snapshot;
        const uint64_t now_us = (uint64_t)esp_timer_get_time();

        metrics_copy_snapshot(&g_project_context, &snapshot);
        log_queue_status(&g_project_context);

        if (g_project_context.mode == PROJECT_MODE_SAMPLING_BENCHMARK) {
            ESP_LOGI(TAG,
                     "Supervisor heartbeat | events=0x%02X mode=%s stage=%" PRIu32 "/%" PRIu32
                     " target=%.1fHz generated=%" PRIu32 " consumed=%" PRIu32
                     " drops=%" PRIu32 " misses=%" PRIu32 " stable=%u complete=%u",
                     (unsigned)bits,
                     project_mode_to_string(g_project_context.mode),
                     g_project_context.benchmark.stage_index + 1U,
                     g_project_context.benchmark.total_stages,
                     (double)g_project_context.benchmark.target_frequency_hz,
                     g_project_context.benchmark.samples_generated_in_stage,
                     g_project_context.benchmark.samples_consumed_in_stage,
                     g_project_context.benchmark.queue_drops_in_stage,
                     g_project_context.benchmark.deadline_misses_in_stage,
                     (unsigned)g_project_context.benchmark.last_stage_stable,
                     (unsigned)g_project_context.benchmark.benchmark_complete);
        } else if (g_project_context.mode == PROJECT_MODE_RAW_SAMPLING_BENCHMARK) {
            ESP_LOGI(TAG,
                     "Supervisor heartbeat | events=0x%02X mode=%s generated=%" PRIu32
                     " achieved=%.2fHz stable=%u complete=%u",
                     (unsigned)bits,
                     project_mode_to_string(g_project_context.mode),
                     g_project_context.benchmark.samples_generated_in_stage,
                     (double)g_project_context.benchmark.achieved_frequency_hz,
                     (unsigned)g_project_context.benchmark.last_stage_stable,
                     (unsigned)g_project_context.benchmark.benchmark_complete);
        } else {
            if (g_project_context.latest_aggregate_valid != 0U) {
                ESP_LOGI(TAG,
                         "Supervisor heartbeat | events=0x%02X mode=%s samples=%" PRIu32
                         " windows=%" PRIu32 " aggregates=%" PRIu32
                         " mqtt=%" PRIu32 " lora=%" PRIu32 "/%" PRIu32 " profile=%s"
                         " fs=%.1fHz dominant=%.2fHz avg=%.4f updates=%" PRIu32,
                         (unsigned)bits,
                         project_mode_to_string(g_project_context.mode),
                         snapshot.samples_generated,
                         snapshot.windows_processed,
                         snapshot.aggregates_processed,
                         snapshot.mqtt_messages_prepared,
                         snapshot.lora_messages_prepared,
                         snapshot.lora_messages_sent,
                         project_signal_profile_name(g_project_context.latest_aggregate.signal_profile),
                         (double)g_project_context.adaptive.current_sampling_frequency_hz,
                         (double)g_project_context.adaptive.last_dominant_frequency_hz,
                         (double)g_project_context.latest_aggregate.average_value,
                         g_project_context.adaptive.frequency_updates);
            } else {
                ESP_LOGI(TAG,
                         "Supervisor heartbeat | events=0x%02X mode=%s samples=%" PRIu32
                         " windows=%" PRIu32 " aggregates=%" PRIu32
                         " mqtt=%" PRIu32 " lora=%" PRIu32 "/%" PRIu32 " profile=%s"
                         " fs=%.1fHz dominant=%.2fHz updates=%" PRIu32,
                         (unsigned)bits,
                         project_mode_to_string(g_project_context.mode),
                         snapshot.samples_generated,
                         snapshot.windows_processed,
                         snapshot.aggregates_processed,
                         snapshot.mqtt_messages_prepared,
                         snapshot.lora_messages_prepared,
                         snapshot.lora_messages_sent,
                         project_signal_profile_name(g_project_context.signal_profile),
                         (double)g_project_context.adaptive.current_sampling_frequency_hz,
                         (double)g_project_context.adaptive.last_dominant_frequency_hz,
                         g_project_context.adaptive.frequency_updates);
            }
        }

        if (project_deep_sleep_ready(&g_project_context, &snapshot, now_us)) {
            project_enter_deep_sleep(&g_project_context, &snapshot);
        }

        vTaskDelay(pdMS_TO_TICKS(PROJECT_SUPERVISOR_HEARTBEAT_MS));
    }
}
