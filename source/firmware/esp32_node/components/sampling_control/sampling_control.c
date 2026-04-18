#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "project_config.h"
#include "project_types.h"
#include "sampling_control.h"

static const char *TAG = "sampling_control";

static float clamp_frequency(float value_hz, float min_hz, float max_hz)
{
    if (value_hz < min_hz) {
        return min_hz;
    }
    if (value_hz > max_hz) {
        return max_hz;
    }
    return value_hz;
}

static float round_frequency_to_step(float frequency_hz, float step_hz)
{
    if (step_hz <= 0.0f) {
        return frequency_hz;
    }

    return roundf(frequency_hz / step_hz) * step_hz;
}

static float select_adaptive_sampling_frequency(float dominant_frequency_hz)
{
    if (dominant_frequency_hz <= 0.0f) {
        return PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ;
    }

    // The policy is intentionally simple: oversample the detected dominant frequency,
    // then snap the result to a small set of practical rates.
    float proposed_frequency_hz =
        dominant_frequency_hz * PROJECT_ADAPTIVE_OVERSAMPLING_FACTOR;
    proposed_frequency_hz =
        round_frequency_to_step(proposed_frequency_hz, PROJECT_ADAPTIVE_RATE_STEP_HZ);

    return clamp_frequency(proposed_frequency_hz,
                           PROJECT_ADAPTIVE_MIN_SAMPLING_FREQUENCY_HZ,
                           PROJECT_ADAPTIVE_MAX_SAMPLING_FREQUENCY_HZ);
}

esp_err_t sampling_control_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL || ctx->fft_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_SAMPLING_CONTROL_READY);
    if (PROJECT_ENABLE_ADAPTIVE_SAMPLING != 0U) {
        ESP_LOGI(TAG,
                 "Sampling control module ready with adaptive policy"
                 " (%.1fx dominant frequency, min %.1f Hz, max %.1f Hz)",
                 (double)PROJECT_ADAPTIVE_OVERSAMPLING_FACTOR,
                 (double)PROJECT_ADAPTIVE_MIN_SAMPLING_FREQUENCY_HZ,
                 (double)PROJECT_ADAPTIVE_MAX_SAMPLING_FREQUENCY_HZ);
    } else {
        ESP_LOGI(TAG,
                 "Sampling control module ready in fixed baseline mode"
                 " (holding %.1f Hz)",
                 (double)PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ);
    }
    return ESP_OK;
}

void sampling_control_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;
    fft_result_t fft_result;

    for (;;) {
        if (xQueueReceive(ctx->fft_queue, &fft_result, pdMS_TO_TICKS(PROJECT_CONTROL_HEARTBEAT_MS)) == pdPASS) {
            const float previous_frequency_hz = ctx->adaptive.current_sampling_frequency_hz;
            const float requested_frequency_hz =
                select_adaptive_sampling_frequency(fft_result.dominant_frequency_hz);

            ctx->adaptive.last_window_id = fft_result.window_id;
            ctx->adaptive.last_dominant_frequency_hz = fft_result.dominant_frequency_hz;
            ctx->adaptive.last_peak_magnitude = fft_result.peak_magnitude;
            ctx->adaptive.last_requested_sampling_frequency_hz =
                (PROJECT_ENABLE_ADAPTIVE_SAMPLING != 0U) ? requested_frequency_hz
                                                         : previous_frequency_hz;

            if (PROJECT_ENABLE_ADAPTIVE_SAMPLING == 0U) {
                // This fixed-rate path is used for baseline energy and timing runs so the
                // same firmware can be reused without removing the FFT stage entirely.
                ESP_LOGI(TAG,
                         "Baseline sampling hold | window=%" PRIu32
                         " dominant=%.2f Hz peak=%.4f fixed_fs=%.1f Hz",
                         fft_result.window_id,
                         (double)fft_result.dominant_frequency_hz,
                         (double)fft_result.peak_magnitude,
                         (double)previous_frequency_hz);
                continue;
            }

            if (fabsf(requested_frequency_hz - previous_frequency_hz) >=
                PROJECT_ADAPTIVE_CHANGE_THRESHOLD_HZ) {
                // Only commit a change when it is large enough to matter; this avoids
                // oscillating the sampler because of tiny FFT variations between windows.
                ctx->adaptive.current_sampling_frequency_hz = requested_frequency_hz;
                ctx->adaptive.frequency_updates++;
                ctx->adaptive.last_update_us = (uint64_t)esp_timer_get_time();

                ESP_LOGI(TAG,
                         "Adaptive sampling update | window=%" PRIu32
                         " dominant=%.2f Hz peak=%.4f previous_fs=%.1f Hz"
                         " new_fs=%.1f Hz",
                         fft_result.window_id,
                         (double)fft_result.dominant_frequency_hz,
                         (double)fft_result.peak_magnitude,
                         (double)previous_frequency_hz,
                         (double)requested_frequency_hz);
            } else {
                ESP_LOGI(TAG,
                         "Adaptive sampling hold | window=%" PRIu32
                         " dominant=%.2f Hz peak=%.4f fs=%.1f Hz",
                         fft_result.window_id,
                         (double)fft_result.dominant_frequency_hz,
                         (double)fft_result.peak_magnitude,
                         (double)previous_frequency_hz);
            }
        } else {
            ESP_LOGI(TAG, "Adaptive sampling policy waiting for FFT results");
        }
    }
}
