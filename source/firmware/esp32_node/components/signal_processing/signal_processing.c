#include <inttypes.h>
#include <float.h>
#include <math.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "project_config.h"
#include "project_types.h"
#include "signal_processing.h"

static const char *TAG = "signal_processing";
static float s_window_buffer[PROJECT_SIGNAL_WINDOW_MAX_SAMPLES];

static bool benchmark_mode_uses_processing_drain(project_mode_t mode)
{
    return mode == PROJECT_MODE_SAMPLING_BENCHMARK ||
           mode == PROJECT_MODE_RAW_SAMPLING_BENCHMARK;
}

typedef struct {
    uint32_t bin_index;
    float frequency_hz;
    float magnitude;
} spectral_peak_t;

static void reset_spectral_peaks(spectral_peak_t *peaks, size_t peak_count)
{
    for (size_t index = 0; index < peak_count; ++index) {
        peaks[index].bin_index = 0U;
        peaks[index].frequency_hz = 0.0f;
        peaks[index].magnitude = -1.0f;
    }
}

static void maybe_insert_peak(spectral_peak_t *peaks,
                              size_t peak_count,
                              uint32_t bin_index,
                              float frequency_hz,
                              float magnitude)
{
    for (size_t index = 0; index < peak_count; ++index) {
        if (magnitude <= peaks[index].magnitude) {
            continue;
        }

        for (size_t shift = peak_count - 1U; shift > index; --shift) {
            peaks[shift] = peaks[shift - 1U];
        }

        peaks[index].bin_index = bin_index;
        peaks[index].frequency_hz = frequency_hz;
        peaks[index].magnitude = magnitude;
        return;
    }
}

static float compute_dft_bin_magnitude(const float *samples,
                                       uint32_t sample_count,
                                       uint32_t bin_index,
                                       float sample_mean)
{
    // We remove the mean first so the DC offset does not dominate the "real" frequencies
    // we care about for adaptive sampling.
    float real = 0.0f;
    float imag = 0.0f;
    const float angle_step = PROJECT_TWO_PI_F * (float)bin_index / (float)sample_count;

    for (uint32_t sample_index = 0; sample_index < sample_count; ++sample_index) {
        const float angle = angle_step * (float)sample_index;
        const float centered_value = samples[sample_index] - sample_mean;

        real += centered_value * cosf(angle);
        imag -= centered_value * sinf(angle);
    }

    return sqrtf((real * real) + (imag * imag)) / (float)sample_count;
}

static void analyse_window_spectrum(const float *samples,
                                    uint32_t sample_count,
                                    float sampling_frequency_hz,
                                    float sample_mean,
                                    fft_result_t *result,
                                    spectral_peak_t *peaks,
                                    size_t peak_count,
                                    float *term1_magnitude,
                                    float *term2_magnitude)
{
    const uint32_t max_bin = sample_count / 2U;
    const uint32_t term1_bin =
        (uint32_t)lroundf((PROJECT_SIGNAL_TERM1_FREQUENCY_HZ * (float)sample_count) / sampling_frequency_hz);
    const uint32_t term2_bin =
        (uint32_t)lroundf((PROJECT_SIGNAL_TERM2_FREQUENCY_HZ * (float)sample_count) / sampling_frequency_hz);

    result->dominant_frequency_hz = 0.0f;
    result->peak_magnitude = 0.0f;
    *term1_magnitude = 0.0f;
    *term2_magnitude = 0.0f;

    reset_spectral_peaks(peaks, peak_count);

    for (uint32_t bin_index = 1U; bin_index <= max_bin; ++bin_index) {
        const float frequency_hz = ((float)bin_index * sampling_frequency_hz) / (float)sample_count;
        const float magnitude = compute_dft_bin_magnitude(samples, sample_count, bin_index, sample_mean);

        if (bin_index == term1_bin) {
            *term1_magnitude = magnitude;
        }
        if (bin_index == term2_bin) {
            *term2_magnitude = magnitude;
        }

        maybe_insert_peak(peaks, peak_count, bin_index, frequency_hz, magnitude);

        if (magnitude > result->peak_magnitude) {
            result->peak_magnitude = magnitude;
            result->dominant_frequency_hz = frequency_hz;
        }
    }
}

static uint32_t compute_window_sample_count(float sampling_frequency_hz)
{
    uint32_t sample_count =
        (uint32_t)lroundf(sampling_frequency_hz * (float)PROJECT_SIGNAL_WINDOW_DURATION_SEC);

    if (sample_count == 0U) {
        sample_count = 1U;
    }
    if (sample_count > PROJECT_SIGNAL_WINDOW_MAX_SAMPLES) {
        sample_count = PROJECT_SIGNAL_WINDOW_MAX_SAMPLES;
    }

    return sample_count;
}

static void fan_out_aggregate_result(project_context_t *ctx, const aggregate_result_t *aggregate)
{
    // MQTT and LoRaWAN consume the same aggregate independently, so each path gets
    // its own queue and can lag or be disabled without blocking the other one.
    if (project_comm_mode_supports_mqtt(ctx->communication_mode) != 0U) {
        if (xQueueSend(ctx->aggregate_mqtt_queue, aggregate, pdMS_TO_TICKS(50)) != pdPASS) {
            ESP_LOGW(TAG, "MQTT aggregate queue full; aggregate result dropped");
        }
    }

    if (project_comm_mode_supports_lorawan(ctx->communication_mode) != 0U) {
        if (xQueueSend(ctx->aggregate_lorawan_queue, aggregate, pdMS_TO_TICKS(50)) != pdPASS) {
            ESP_LOGW(TAG, "LoRaWAN aggregate queue full; aggregate result dropped");
        }
    }
}

esp_err_t signal_processing_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL || ctx->sample_queue == NULL ||
        ctx->fft_queue == NULL || ctx->aggregate_mqtt_queue == NULL ||
        ctx->aggregate_lorawan_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_SIGNAL_PROCESSING_READY);
    if (benchmark_mode_uses_processing_drain(ctx->mode)) {
        ESP_LOGI(TAG, "Signal processing module ready in benchmark drain mode");
    } else {
        ESP_LOGI(TAG,
                 "Signal processing module ready with adaptive spectral windows"
                 " (initially %u samples at %.1f Hz)",
                 (unsigned)compute_window_sample_count(PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ),
                 (double)PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ);
    }
    return ESP_OK;
}

void signal_processing_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;
    raw_sample_t sample;

    if (benchmark_mode_uses_processing_drain(ctx->mode)) {
        uint64_t last_report_us = (uint64_t)esp_timer_get_time();

        for (;;) {
            if (xQueueReceive(ctx->sample_queue, &sample, pdMS_TO_TICKS(PROJECT_PROCESSING_TIMEOUT_MS)) == pdPASS) {
                ctx->benchmark.samples_consumed_in_stage++;
            }

            const uint64_t now_us = (uint64_t)esp_timer_get_time();
            if ((now_us - last_report_us) >= (uint64_t)PROJECT_BENCHMARK_PROGRESS_REPORT_MS * 1000ULL) {
                ESP_LOGI(TAG,
                         "Benchmark drain | stage=%" PRIu32 "/%" PRIu32 " consumed=%" PRIu32
                         " queue=%u",
                         ctx->benchmark.stage_index + 1U,
                         ctx->benchmark.total_stages,
                         ctx->benchmark.samples_consumed_in_stage,
                         (unsigned)uxQueueMessagesWaiting(ctx->sample_queue));
                last_report_us = now_us;
            }
        }
    }

    uint32_t processed_samples = 0;
    uint32_t window_count = 0;
    uint32_t window_id = 0;
    uint32_t window_target_samples = compute_window_sample_count(PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ);
    uint64_t window_start_us = 0;
    float window_sampling_frequency_hz = PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ;
    signal_profile_t window_signal_profile = SIGNAL_PROFILE_CLEAN_REFERENCE;
    uint32_t window_anomaly_count = 0U;
    float window_sum = 0.0f;
    float window_min = FLT_MAX;
    float window_max = -FLT_MAX;

    for (;;) {
        if (xQueueReceive(ctx->sample_queue, &sample, pdMS_TO_TICKS(PROJECT_PROCESSING_TIMEOUT_MS)) == pdPASS) {
            processed_samples++;

            if (window_count > 0U &&
                fabsf(sample.sampling_frequency_hz - window_sampling_frequency_hz) >=
                    PROJECT_ADAPTIVE_CHANGE_THRESHOLD_HZ) {
                // Mixing two sampling rates inside one FFT window makes the frequency result
                // hard to trust, so we restart the window when the control task changes rate.
                ESP_LOGW(TAG,
                         "Sampling-rate transition inside window %" PRIu32
                         " after %u/%u samples (%.1f Hz -> %.1f Hz);"
                         " discarding partial window and restarting",
                         window_id,
                         (unsigned)window_count,
                         (unsigned)window_target_samples,
                         (double)window_sampling_frequency_hz,
                         (double)sample.sampling_frequency_hz);
                window_count = 0U;
                window_anomaly_count = 0U;
                window_sum = 0.0f;
                window_min = FLT_MAX;
                window_max = -FLT_MAX;
            }

            if (window_count > 0U && sample.signal_profile != window_signal_profile) {
                ESP_LOGW(TAG,
                         "Signal-profile transition inside window %" PRIu32
                         " after %u/%u samples (%s -> %s); discarding partial window and restarting",
                         window_id,
                         (unsigned)window_count,
                         (unsigned)window_target_samples,
                         project_signal_profile_name(window_signal_profile),
                         project_signal_profile_name(sample.signal_profile));
                window_count = 0U;
                window_anomaly_count = 0U;
                window_sum = 0.0f;
                window_min = FLT_MAX;
                window_max = -FLT_MAX;
            }

            if (window_count == 0U) {
                window_start_us = sample.timestamp_us;
                window_sampling_frequency_hz = sample.sampling_frequency_hz;
                window_target_samples = compute_window_sample_count(window_sampling_frequency_hz);
                window_signal_profile = sample.signal_profile;
                window_anomaly_count = 0U;
                window_sum = 0.0f;
                window_min = FLT_MAX;
                window_max = -FLT_MAX;
            }

            if (window_count < PROJECT_SIGNAL_WINDOW_MAX_SAMPLES) {
                s_window_buffer[window_count] = sample.value;
            }

            window_sum += sample.value;
            if (sample.value < window_min) {
                window_min = sample.value;
            }
            if (sample.value > window_max) {
                window_max = sample.value;
            }

            if (window_count < PROJECT_SIGNAL_PREVIEW_SAMPLES) {
                ESP_LOGI(TAG,
                         "Window %" PRIu32 " sample[%u] = %.4f profile=%s anomaly=%u",
                         window_id,
                         (unsigned)window_count,
                         (double)sample.value,
                         project_signal_profile_name(sample.signal_profile),
                         (unsigned)sample.anomaly_injected);
            }

            window_anomaly_count += sample.anomaly_injected != 0U ? 1U : 0U;
            window_count++;

            if (window_count >= window_target_samples) {
                // A completed window produces two outputs:
                // 1) an FFT result for the control task
                // 2) an aggregate result for the communication tasks
                const float average_value = window_sum / (float)window_count;
                spectral_peak_t peaks[PROJECT_FFT_PEAKS_TO_LOG];
                float term1_magnitude = 0.0f;
                float term2_magnitude = 0.0f;
                fft_result_t result = {
                    .window_id = window_id,
                    .window_start_us = window_start_us,
                    .window_end_us = (uint64_t)esp_timer_get_time(),
                    .dominant_frequency_hz = 0.0f,
                    .peak_magnitude = 0.0f,
                };
                aggregate_result_t aggregate = {
                    .window_id = window_id,
                    .window_start_us = window_start_us,
                    .window_end_us = result.window_end_us,
                    .sample_count = window_count,
                    .sampling_frequency_hz = window_sampling_frequency_hz,
                    .dominant_frequency_hz = 0.0f,
                    .average_value = average_value,
                    .signal_profile = window_signal_profile,
                    .anomaly_count = window_anomaly_count,
                };

                analyse_window_spectrum(s_window_buffer,
                                        window_count,
                                        window_sampling_frequency_hz,
                                        average_value,
                                        &result,
                                        peaks,
                                        PROJECT_FFT_PEAKS_TO_LOG,
                                        &term1_magnitude,
                                        &term2_magnitude);
                aggregate.dominant_frequency_hz = result.dominant_frequency_hz;

                ctx->timing.windows_processed++;
                ctx->timing.last_fft_us = result.window_end_us;
                ctx->timing.aggregates_processed++;
                ctx->timing.last_aggregate_us = result.window_end_us;
                ctx->latest_aggregate = aggregate;
                ctx->latest_aggregate_valid = 1U;

                if (xQueueSend(ctx->fft_queue, &result, 0) == pdPASS) {
                    ESP_LOGI(TAG,
                             "Window %" PRIu32
                             " ready | profile=%s fs=%.1fHz samples=%u anomalies=%" PRIu32
                             " avg=%.4f min=%.4f max=%.4f"
                             " duration=%.2fs",
                             result.window_id,
                             project_signal_profile_name(aggregate.signal_profile),
                             (double)window_sampling_frequency_hz,
                             (unsigned)window_count,
                             aggregate.anomaly_count,
                             (double)average_value,
                             (double)window_min,
                             (double)window_max,
                             (double)((result.window_end_us - window_start_us) / 1000000.0f));
                    ESP_LOGI(TAG,
                             "FFT result | window=%" PRIu32 " dominant=%.2f Hz peak=%.4f"
                             " expected_3Hz=%.4f expected_5Hz=%.4f",
                             result.window_id,
                             (double)result.dominant_frequency_hz,
                             (double)result.peak_magnitude,
                             (double)term1_magnitude,
                             (double)term2_magnitude);

                    for (size_t peak_index = 0; peak_index < PROJECT_FFT_PEAKS_TO_LOG; ++peak_index) {
                        if (peaks[peak_index].magnitude < 0.0f) {
                            continue;
                        }

                        ESP_LOGI(TAG,
                                 "FFT peak[%u] | bin=%" PRIu32 " freq=%.2f Hz magnitude=%.4f",
                                 (unsigned)peak_index,
                                 peaks[peak_index].bin_index,
                                 (double)peaks[peak_index].frequency_hz,
                                 (double)peaks[peak_index].magnitude);
                    }
                } else {
                    ESP_LOGW(TAG, "FFT queue full; spectral result dropped");
                }

                fan_out_aggregate_result(ctx, &aggregate);

                window_id++;
                window_count = 0;
            }
        } else {
            ESP_LOGI(TAG,
                     "Waiting for samples; processed=%" PRIu32 " current_window=%u/%u",
                     processed_samples,
                     (unsigned)window_count,
                     (unsigned)window_target_samples);
        }
    }
}
