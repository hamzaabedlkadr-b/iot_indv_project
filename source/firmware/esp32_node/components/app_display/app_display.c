#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "app_display.h"
#include "project_config.h"
#include "project_types.h"

static const char *TAG = "app_display";

esp_err_t app_display_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_DISPLAY_READY);
    ESP_LOGI(TAG, "Display module ready with serial-only placeholder");
    return ESP_OK;
}

void app_display_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;

    for (;;) {
        if (ctx->latest_aggregate_valid != 0U) {
            ESP_LOGI(TAG,
                     "Display heartbeat | mode=%d events=0x%02X fs=%.1fHz dominant=%.2fHz"
                     " avg=%.4f window=%" PRIu32 " profile=%s anomalies=%" PRIu32,
                     (int)ctx->mode,
                     (unsigned)xEventGroupGetBits(ctx->system_events),
                     (double)ctx->adaptive.current_sampling_frequency_hz,
                     (double)ctx->adaptive.last_dominant_frequency_hz,
                     (double)ctx->latest_aggregate.average_value,
                     ctx->latest_aggregate.window_id,
                     project_signal_profile_name(ctx->latest_aggregate.signal_profile),
                     ctx->latest_aggregate.anomaly_count);
        } else {
            ESP_LOGI(TAG,
                     "Display heartbeat | mode=%d events=0x%02X fs=%.1fHz dominant=%.2fHz profile=%s",
                     (int)ctx->mode,
                     (unsigned)xEventGroupGetBits(ctx->system_events),
                     (double)ctx->adaptive.current_sampling_frequency_hz,
                     (double)ctx->adaptive.last_dominant_frequency_hz,
                     project_signal_profile_name(ctx->signal_profile));
        }

        vTaskDelay(pdMS_TO_TICKS(PROJECT_DISPLAY_HEARTBEAT_MS));
    }
}
