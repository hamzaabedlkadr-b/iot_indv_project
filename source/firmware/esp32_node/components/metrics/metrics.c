#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "metrics.h"
#include "project_config.h"
#include "project_types.h"

static const char *TAG = "metrics";

static bool better_serial_plotter_enabled(void)
{
    return PROJECT_ENABLE_BETTER_SERIAL_PLOTTER != 0U;
}

static void emit_better_serial_plotter_row(const aggregate_result_t *aggregate)
{
    if (!better_serial_plotter_enabled() || aggregate == NULL) {
        return;
    }

    // BetterSerialPlotter can get confused by ESP32 boot/reset text on port open.
    // Keeping the stream to three numeric values makes it much easier to recover
    // and lets the app use its own wall-clock x-axis by default.
    //
    // Column order:
    // sampling_frequency_hz, dominant_frequency_hz, average_value
    printf("%.2f\t%.2f\t%.4f\n",
           (double)aggregate->sampling_frequency_hz,
           (double)aggregate->dominant_frequency_hz,
           (double)aggregate->average_value);
    fflush(stdout);
}

esp_err_t metrics_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(&ctx->timing, 0, sizeof(ctx->timing));
    ctx->timing.boot_time_us = metrics_now_us();

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_METRICS_READY);
    ESP_LOGI(TAG, "Metrics module initialized");
    return ESP_OK;
}

uint64_t metrics_now_us(void)
{
    return (uint64_t)esp_timer_get_time();
}

void metrics_copy_snapshot(const project_context_t *ctx, timing_metrics_t *out_snapshot)
{
    if (ctx == NULL || out_snapshot == NULL) {
        return;
    }

    *out_snapshot = ctx->timing;
}

void metrics_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;
    const uint64_t heartbeat_interval_us = (uint64_t)PROJECT_METRICS_HEARTBEAT_MS * 1000ULL;
    uint64_t last_heartbeat_us = metrics_now_us();
    uint32_t last_logged_window_id = UINT32_MAX;

    for (;;) {
        // We log each finished aggregate once, then keep emitting heartbeats that summarize
        // whether the pipeline is still moving and whether messages are actually leaving it.
        if (ctx->latest_aggregate_valid != 0U &&
            ctx->latest_aggregate.window_id != last_logged_window_id) {
            emit_better_serial_plotter_row(&ctx->latest_aggregate);
            ESP_LOGI(TAG,
                     "Aggregate result | window=%" PRIu32 " profile=%s fs=%.1fHz"
                     " samples=%" PRIu32 " anomalies=%" PRIu32
                     " avg=%.4f dominant=%.2f Hz",
                     ctx->latest_aggregate.window_id,
                     project_signal_profile_name(ctx->latest_aggregate.signal_profile),
                     (double)ctx->latest_aggregate.sampling_frequency_hz,
                     ctx->latest_aggregate.sample_count,
                     ctx->latest_aggregate.anomaly_count,
                     (double)ctx->latest_aggregate.average_value,
                     (double)ctx->latest_aggregate.dominant_frequency_hz);
            last_logged_window_id = ctx->latest_aggregate.window_id;
        }

        const uint64_t now_us = metrics_now_us();
        if ((now_us - last_heartbeat_us) >= heartbeat_interval_us) {
            timing_metrics_t snapshot;
            metrics_copy_snapshot(ctx, &snapshot);

            if (ctx->latest_aggregate_valid != 0U) {
                ESP_LOGI(TAG,
                         "Metrics heartbeat | uptime_us=%llu last_sample=%llu last_fft=%llu"
                         " last_aggregate=%llu last_publish=%llu aggregates=%" PRIu32
                         " latest_window=%" PRIu32 " profile=%s anomalies=%" PRIu32
                         " avg=%.4f edge_latency_us=%llu"
                         " mqtt_sent=%" PRIu32 " lora_prepared=%" PRIu32
                         " lora_sent=%" PRIu32,
                         snapshot.boot_time_us == 0 ? 0ULL : now_us - snapshot.boot_time_us,
                         snapshot.last_sample_us,
                         snapshot.last_fft_us,
                         snapshot.last_aggregate_us,
                         snapshot.last_publish_us,
                         snapshot.aggregates_processed,
                         ctx->latest_aggregate.window_id,
                         project_signal_profile_name(ctx->latest_aggregate.signal_profile),
                         ctx->latest_aggregate.anomaly_count,
                         (double)ctx->latest_aggregate.average_value,
                         snapshot.last_edge_latency_us,
                         snapshot.mqtt_messages_sent,
                         snapshot.lora_messages_prepared,
                         snapshot.lora_messages_sent);
            } else {
                ESP_LOGI(TAG,
                         "Metrics heartbeat | uptime_us=%llu last_sample=%llu last_fft=%llu"
                         " last_aggregate=%llu last_publish=%llu aggregates=%" PRIu32
                         " edge_latency_us=%llu mqtt_sent=%" PRIu32
                         " lora_prepared=%" PRIu32 " lora_sent=%" PRIu32,
                         snapshot.boot_time_us == 0 ? 0ULL : now_us - snapshot.boot_time_us,
                         snapshot.last_sample_us,
                         snapshot.last_fft_us,
                         snapshot.last_aggregate_us,
                         snapshot.last_publish_us,
                         snapshot.aggregates_processed,
                         snapshot.last_edge_latency_us,
                         snapshot.mqtt_messages_sent,
                         snapshot.lora_messages_prepared,
                         snapshot.lora_messages_sent);
            }

            last_heartbeat_us = now_us;
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
