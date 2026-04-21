#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "project_config.h"
#include "project_types.h"
#include "signal_input.h"

static const char *TAG = "signal_input";
static const float s_benchmark_frequencies_hz[] = {
    50.0f,
    100.0f,
    200.0f,
    250.0f,
    500.0f,
    1000.0f,
};

static uint32_t xorshift32_next(uint32_t *state)
{
    uint32_t value = *state;

    if (value == 0U) {
        value = PROJECT_SIGNAL_RANDOM_SEED;
    }

    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;

    *state = value;
    return value;
}

static float random_uniform_01(uint32_t *state)
{
    return (float)(xorshift32_next(state) & 0x00FFFFFFU) / 16777216.0f;
}

static float random_gaussian_like(uint32_t *state, float sigma)
{
    float sum = 0.0f;

    // Twelve uniforms minus six gives a simple near-Gaussian sample with variance near one.
    for (size_t index = 0; index < 12U; ++index) {
        sum += random_uniform_01(state);
    }

    return (sum - 6.0f) * sigma;
}

static float maybe_generate_anomaly(uint32_t *state, uint8_t *out_injected)
{
    if (out_injected != NULL) {
        *out_injected = 0U;
    }

    if (random_uniform_01(state) >= PROJECT_SIGNAL_ANOMALY_PROBABILITY) {
        return 0.0f;
    }

    const float magnitude_span =
        PROJECT_SIGNAL_ANOMALY_MAX_MAGNITUDE - PROJECT_SIGNAL_ANOMALY_MIN_MAGNITUDE;
    const float magnitude =
        PROJECT_SIGNAL_ANOMALY_MIN_MAGNITUDE + (random_uniform_01(state) * magnitude_span);
    const float sign = random_uniform_01(state) < 0.5f ? -1.0f : 1.0f;

    if (out_injected != NULL) {
        *out_injected = 1U;
    }

    return sign * magnitude;
}

static float generate_base_signal(float time_seconds)
{
    const float term_1 =
        PROJECT_SIGNAL_TERM1_AMPLITUDE *
        sinf(PROJECT_TWO_PI_F * PROJECT_SIGNAL_TERM1_FREQUENCY_HZ * time_seconds);
    const float term_2 =
        PROJECT_SIGNAL_TERM2_AMPLITUDE *
        sinf(PROJECT_TWO_PI_F * PROJECT_SIGNAL_TERM2_FREQUENCY_HZ * time_seconds);

    return term_1 + term_2;
}

static float generate_profiled_signal(signal_profile_t profile,
                                      float time_seconds,
                                      uint32_t *rng_state,
                                      uint8_t *out_anomaly_injected,
                                      float *out_anomaly_component)
{
    const float base_signal = generate_base_signal(time_seconds);
    float noise_component = 0.0f;
    float anomaly_component = 0.0f;
    uint8_t anomaly_injected = 0U;

    switch (profile) {
        case SIGNAL_PROFILE_NOISY_REFERENCE:
            noise_component = random_gaussian_like(rng_state, PROJECT_SIGNAL_NOISE_SIGMA);
            break;
        case SIGNAL_PROFILE_ANOMALY_STRESS:
            noise_component = random_gaussian_like(rng_state, PROJECT_SIGNAL_NOISE_SIGMA);
            anomaly_component = maybe_generate_anomaly(rng_state, &anomaly_injected);
            break;
        case SIGNAL_PROFILE_CLEAN_REFERENCE:
        default:
            break;
    }

    if (out_anomaly_injected != NULL) {
        *out_anomaly_injected = anomaly_injected;
    }
    if (out_anomaly_component != NULL) {
        *out_anomaly_component = anomaly_component;
    }

    return base_signal + noise_component + anomaly_component;
}

static const char *signal_input_mode_name(project_mode_t mode)
{
    switch (mode) {
        case PROJECT_MODE_VIRTUAL_SENSOR:
            return "virtual_sensor";
        case PROJECT_MODE_SAMPLING_BENCHMARK:
            return "sampling_benchmark";
        case PROJECT_MODE_RAW_SAMPLING_BENCHMARK:
            return "raw_sampling_benchmark";
        case PROJECT_MODE_HC_SR04_DEMO:
            return "hc_sr04_demo";
        case PROJECT_MODE_PHASE2_SKELETON:
            return "phase2_skeleton";
        default:
            return "unknown";
    }
}

static bool benchmark_stage_is_stable(const sampling_benchmark_metrics_t *metrics)
{
    return metrics->queue_drops_in_stage <= PROJECT_BENCHMARK_QUEUE_DROP_THRESHOLD &&
           metrics->deadline_misses_in_stage == 0U;
}

static float current_virtual_sampling_frequency_hz(const project_context_t *ctx)
{
    const float configured_frequency = ctx->adaptive.current_sampling_frequency_hz;

    if (configured_frequency < PROJECT_ADAPTIVE_MIN_SAMPLING_FREQUENCY_HZ ||
        configured_frequency > PROJECT_ADAPTIVE_MAX_SAMPLING_FREQUENCY_HZ) {
        return PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ;
    }

    return configured_frequency;
}

static void benchmark_wait_until(uint64_t target_time_us)
{
    for (;;) {
        const uint64_t now_us = (uint64_t)esp_timer_get_time();

        if (now_us >= target_time_us) {
            return;
        }

        const uint64_t remaining_us = target_time_us - now_us;
        if (remaining_us > 2000U) {
            // Sleep while we are far away from the deadline, then switch to a tight wait
            // near the end so the benchmark is not limited by RTOS tick granularity.
            const uint32_t coarse_delay_ms = (uint32_t)(remaining_us / 1000U) - 1U;

            if (coarse_delay_ms > 0U) {
                vTaskDelay(pdMS_TO_TICKS(coarse_delay_ms));
                continue;
            }
        }

        while ((uint64_t)esp_timer_get_time() < target_time_us) {
        }
        return;
    }
}

static void benchmark_reset_stage(project_context_t *ctx, size_t stage_index, float target_frequency_hz)
{
    ctx->benchmark.stage_index = (uint32_t)stage_index;
    ctx->benchmark.total_stages = (uint32_t)(sizeof(s_benchmark_frequencies_hz) / sizeof(s_benchmark_frequencies_hz[0]));
    ctx->benchmark.samples_generated_in_stage = 0U;
    ctx->benchmark.samples_consumed_in_stage = 0U;
    ctx->benchmark.queue_drops_in_stage = 0U;
    ctx->benchmark.deadline_misses_in_stage = 0U;
    ctx->benchmark.target_frequency_hz = target_frequency_hz;
    ctx->benchmark.achieved_frequency_hz = 0.0f;
    ctx->benchmark.min_interval_us = 0.0f;
    ctx->benchmark.max_interval_us = 0.0f;
    ctx->benchmark.worst_lateness_us = 0;
    ctx->benchmark.stage_start_us = (uint64_t)esp_timer_get_time();
    ctx->benchmark.last_stage_duration_us = 0U;
    ctx->benchmark.stage_completed = 0U;

    ESP_LOGI(TAG,
             "Benchmark stage %u/%u started at %.1f Hz",
             (unsigned)(ctx->benchmark.stage_index + 1U),
             (unsigned)ctx->benchmark.total_stages,
             (double)ctx->benchmark.target_frequency_hz);
}

static void benchmark_finish_stage(project_context_t *ctx, uint64_t stage_end_us)
{
    const uint64_t duration_us = stage_end_us - ctx->benchmark.stage_start_us;
    const float duration_seconds = (float)duration_us / 1000000.0f;

    ctx->benchmark.last_stage_duration_us = duration_us;
    ctx->benchmark.achieved_frequency_hz = duration_seconds > 0.0f
                                               ? (float)ctx->benchmark.samples_generated_in_stage / duration_seconds
                                               : 0.0f;
    ctx->benchmark.last_stage_stable = benchmark_stage_is_stable(&ctx->benchmark) ? 1U : 0U;
    ctx->benchmark.stage_completed = 1U;

    ESP_LOGI(TAG,
             "Benchmark result | stage=%u/%u target=%.1fHz achieved=%.2fHz"
             " samples=%" PRIu32 " consumed=%" PRIu32 " drops=%" PRIu32
             " misses=%" PRIu32 " min_dt=%.1fus max_dt=%.1fus worst_late=%" PRIi32
             " stable=%s duration=%.2fs",
             (unsigned)(ctx->benchmark.stage_index + 1U),
             (unsigned)ctx->benchmark.total_stages,
             (double)ctx->benchmark.target_frequency_hz,
             (double)ctx->benchmark.achieved_frequency_hz,
             ctx->benchmark.samples_generated_in_stage,
             ctx->benchmark.samples_consumed_in_stage,
             ctx->benchmark.queue_drops_in_stage,
             ctx->benchmark.deadline_misses_in_stage,
             (double)ctx->benchmark.min_interval_us,
             (double)ctx->benchmark.max_interval_us,
             ctx->benchmark.worst_lateness_us,
             ctx->benchmark.last_stage_stable ? "yes" : "no",
             (double)duration_seconds);
}

static void benchmark_reset_raw_mode(project_context_t *ctx)
{
    ctx->benchmark.stage_index = 0U;
    ctx->benchmark.total_stages = 1U;
    ctx->benchmark.samples_generated_in_stage = 0U;
    ctx->benchmark.samples_consumed_in_stage = 0U;
    ctx->benchmark.queue_drops_in_stage = 0U;
    ctx->benchmark.deadline_misses_in_stage = 0U;
    ctx->benchmark.target_frequency_hz = 0.0f;
    ctx->benchmark.achieved_frequency_hz = 0.0f;
    ctx->benchmark.min_interval_us = 0.0f;
    ctx->benchmark.max_interval_us = 0.0f;
    ctx->benchmark.worst_lateness_us = 0;
    ctx->benchmark.stage_start_us = (uint64_t)esp_timer_get_time();
    ctx->benchmark.last_stage_duration_us = 0U;
    ctx->benchmark.stage_completed = 0U;
    ctx->benchmark.benchmark_complete = 0U;
    ctx->benchmark.last_stage_stable = 0U;

    ESP_LOGI(TAG,
             "Raw sampling benchmark started for %u ms",
             (unsigned)PROJECT_RAW_BENCHMARK_DURATION_MS);
}

static void benchmark_finish_raw_mode(project_context_t *ctx, uint64_t stage_end_us)
{
    const uint64_t duration_us = stage_end_us - ctx->benchmark.stage_start_us;
    const float duration_seconds = (float)duration_us / 1000000.0f;

    ctx->benchmark.last_stage_duration_us = duration_us;
    ctx->benchmark.achieved_frequency_hz = duration_seconds > 0.0f
                                               ? (float)ctx->benchmark.samples_generated_in_stage / duration_seconds
                                               : 0.0f;
    ctx->benchmark.last_stage_stable = 1U;
    ctx->benchmark.stage_completed = 1U;
    ctx->benchmark.benchmark_complete = 1U;

    ESP_LOGI(TAG,
             "Raw benchmark result | generated=%" PRIu32 " min_dt=%.1fus max_dt=%.1fus"
             " achieved=%.2fHz stable=%s duration=%.2fs",
             ctx->benchmark.samples_generated_in_stage,
             (double)ctx->benchmark.min_interval_us,
             (double)ctx->benchmark.max_interval_us,
             (double)ctx->benchmark.achieved_frequency_hz,
             ctx->benchmark.last_stage_stable ? "yes" : "no",
             (double)duration_seconds);
}

static void run_virtual_sensor_mode(project_context_t *ctx)
{
    const uint64_t start_time_us = (uint64_t)esp_timer_get_time();
    uint64_t next_sample_due_us = start_time_us;
    uint32_t sample_id = 0;
    uint32_t rng_state = PROJECT_SIGNAL_RANDOM_SEED;
    float last_reported_frequency_hz = current_virtual_sampling_frequency_hz(ctx);

    ESP_LOGI(TAG,
             "Virtual sensor adaptive sampling starts at %.1f Hz with profile %s",
             (double)last_reported_frequency_hz,
             project_signal_profile_name(ctx->signal_profile));

    for (;;) {
        // The input task reads the latest adaptive rate every cycle, so sampling changes
        // naturally at the source instead of forcing downstream tasks to resample.
        const float active_frequency_hz = current_virtual_sampling_frequency_hz(ctx);
        const uint64_t scheduled_time_us = next_sample_due_us;
        const uint64_t period_us = (uint64_t)((1000000.0f / active_frequency_hz) + 0.5f);

        if (fabsf(active_frequency_hz - last_reported_frequency_hz) >= PROJECT_ADAPTIVE_CHANGE_THRESHOLD_HZ) {
            ESP_LOGI(TAG,
                     "Applying adaptive sampling frequency %.1f Hz",
                     (double)active_frequency_hz);
            last_reported_frequency_hz = active_frequency_hz;
        }

        benchmark_wait_until(scheduled_time_us);

        const uint64_t now_us = (uint64_t)esp_timer_get_time();
        const float elapsed_seconds = (float)(now_us - start_time_us) / 1000000.0f;
        float anomaly_component = 0.0f;
        uint8_t anomaly_injected = 0U;
        raw_sample_t sample = {
            .sample_id = sample_id++,
            .timestamp_us = now_us,
            .value = generate_profiled_signal(ctx->signal_profile,
                                              elapsed_seconds,
                                              &rng_state,
                                              &anomaly_injected,
                                              &anomaly_component),
            .sampling_frequency_hz = active_frequency_hz,
            .source = SENSOR_SOURCE_VIRTUAL,
            .signal_profile = ctx->signal_profile,
            .anomaly_injected = anomaly_injected,
        };

        if (xQueueSend(ctx->sample_queue, &sample, pdMS_TO_TICKS(25)) == pdPASS) {
            ctx->timing.samples_generated++;
            ctx->timing.last_sample_us = sample.timestamp_us;

            if ((sample.sample_id % PROJECT_SIGNAL_INPUT_LOG_INTERVAL) == 0U) {
                ESP_LOGI(TAG,
                         "Generated sample id=%" PRIu32 " t=%.3fs value=%.4f fs=%.1fHz",
                         sample.sample_id,
                         (double)elapsed_seconds,
                         (double)sample.value,
                         (double)sample.sampling_frequency_hz);
            }

            if (sample.anomaly_injected != 0U) {
                ESP_LOGW(TAG,
                         "Injected anomaly | profile=%s sample_id=%" PRIu32
                         " t=%.3fs anomaly=%.4f value=%.4f",
                         project_signal_profile_name(sample.signal_profile),
                         sample.sample_id,
                         (double)elapsed_seconds,
                         (double)anomaly_component,
                         (double)sample.value);
            }
        } else {
            ESP_LOGW(TAG, "Sample queue full; virtual sample dropped");
        }

        next_sample_due_us = scheduled_time_us + period_us;
    }
}

static void run_sampling_benchmark_mode(project_context_t *ctx)
{
    const size_t stage_count = sizeof(s_benchmark_frequencies_hz) / sizeof(s_benchmark_frequencies_hz[0]);
    const uint64_t benchmark_start_us = (uint64_t)esp_timer_get_time();
    uint32_t global_sample_id = 0U;
    uint32_t rng_state = PROJECT_SIGNAL_RANDOM_SEED;

    for (size_t stage_index = 0; stage_index < stage_count; ++stage_index) {
        const float frequency_hz = s_benchmark_frequencies_hz[stage_index];
        const float expected_interval_us = 1000000.0f / frequency_hz;
        const uint64_t period_us = (uint64_t)(expected_interval_us + 0.5f);
        const uint64_t stage_duration_us = (uint64_t)PROJECT_BENCHMARK_STAGE_DURATION_MS * 1000ULL;
        uint64_t next_sample_due_us = 0U;
        uint64_t last_sample_timestamp_us = 0U;
        uint64_t last_progress_report_us = 0U;

        benchmark_reset_stage(ctx, stage_index, frequency_hz);
        last_progress_report_us = ctx->benchmark.stage_start_us;
        next_sample_due_us = ctx->benchmark.stage_start_us;

        for (;;) {
            benchmark_wait_until(next_sample_due_us);

            const uint64_t now_us = (uint64_t)esp_timer_get_time();
            const uint64_t scheduled_time_us = next_sample_due_us;
            const uint64_t stage_elapsed_us = now_us - ctx->benchmark.stage_start_us;
            const float elapsed_seconds = (float)(now_us - benchmark_start_us) / 1000000.0f;
            uint8_t anomaly_injected = 0U;
            raw_sample_t sample = {
                .sample_id = global_sample_id++,
                .timestamp_us = now_us,
                .value = generate_profiled_signal(ctx->signal_profile,
                                                  elapsed_seconds,
                                                  &rng_state,
                                                  &anomaly_injected,
                                                  NULL),
                .sampling_frequency_hz = frequency_hz,
                .source = SENSOR_SOURCE_VIRTUAL,
                .signal_profile = ctx->signal_profile,
                .anomaly_injected = anomaly_injected,
            };

            if (ctx->benchmark.samples_generated_in_stage == 0U) {
                ctx->benchmark.min_interval_us = expected_interval_us;
                ctx->benchmark.max_interval_us = expected_interval_us;
            } else {
                // We track both interval spread and lateness so the result says more than
                // "it ran" or "it crashed"; it tells us how cleanly the stage held timing.
                const float interval_us = (float)(now_us - last_sample_timestamp_us);
                const int32_t lateness_us = (int32_t)(now_us - scheduled_time_us);

                if (interval_us < ctx->benchmark.min_interval_us) {
                    ctx->benchmark.min_interval_us = interval_us;
                }
                if (interval_us > ctx->benchmark.max_interval_us) {
                    ctx->benchmark.max_interval_us = interval_us;
                }
                if (lateness_us > ctx->benchmark.worst_lateness_us) {
                    ctx->benchmark.worst_lateness_us = lateness_us;
                }
                if (lateness_us > (int32_t)PROJECT_BENCHMARK_DEADLINE_MISS_TOLERANCE_US) {
                    ctx->benchmark.deadline_misses_in_stage++;
                }
            }

            last_sample_timestamp_us = now_us;
            next_sample_due_us = scheduled_time_us + period_us;

            if (xQueueSend(ctx->sample_queue, &sample, 0) == pdPASS) {
                ctx->timing.samples_generated++;
                ctx->timing.last_sample_us = sample.timestamp_us;
                ctx->benchmark.samples_generated_in_stage++;
            } else {
                ctx->benchmark.queue_drops_in_stage++;
            }

            if ((now_us - last_progress_report_us) >= (uint64_t)PROJECT_BENCHMARK_PROGRESS_REPORT_MS * 1000ULL) {
                ESP_LOGI(TAG,
                         "Benchmark progress | stage=%u/%u target=%.1fHz generated=%" PRIu32
                         " consumed=%" PRIu32 " drops=%" PRIu32 " misses=%" PRIu32
                         " queue=%u elapsed=%.2fs",
                         (unsigned)(ctx->benchmark.stage_index + 1U),
                         (unsigned)ctx->benchmark.total_stages,
                         (double)ctx->benchmark.target_frequency_hz,
                         ctx->benchmark.samples_generated_in_stage,
                         ctx->benchmark.samples_consumed_in_stage,
                         ctx->benchmark.queue_drops_in_stage,
                         ctx->benchmark.deadline_misses_in_stage,
                         (unsigned)uxQueueMessagesWaiting(ctx->sample_queue),
                         (double)((float)stage_elapsed_us / 1000000.0f));
                last_progress_report_us = now_us;
            }

            if (stage_elapsed_us >= stage_duration_us) {
                benchmark_finish_stage(ctx, now_us);
                break;
            }
        }
    }

    ctx->benchmark.benchmark_complete = 1U;
    ESP_LOGI(TAG, "Sampling benchmark completed across all configured stages");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PROJECT_SUPERVISOR_HEARTBEAT_MS));
    }
}

static void run_raw_sampling_benchmark_mode(project_context_t *ctx)
{
    const uint64_t benchmark_duration_us = (uint64_t)PROJECT_RAW_BENCHMARK_DURATION_MS * 1000ULL;
    const uint64_t benchmark_start_us = (uint64_t)esp_timer_get_time();
    uint64_t benchmark_end_us = benchmark_start_us;
    uint64_t last_progress_report_us = benchmark_start_us;
    uint64_t last_sample_timestamp_us = 0U;
    uint32_t global_sample_id = 0U;
    uint32_t rng_state = PROJECT_SIGNAL_RANDOM_SEED;

    benchmark_reset_raw_mode(ctx);

    for (;;) {
        const uint64_t now_us = (uint64_t)esp_timer_get_time();
        const uint64_t elapsed_us = now_us - benchmark_start_us;

        if (elapsed_us >= benchmark_duration_us) {
            benchmark_end_us = now_us;
            break;
        }

        const float elapsed_seconds = (float)elapsed_us / 1000000.0f;
        uint8_t anomaly_injected = 0U;
        raw_sample_t sample = {
            .sample_id = global_sample_id++,
            .timestamp_us = now_us,
            .value = generate_profiled_signal(ctx->signal_profile,
                                              elapsed_seconds,
                                              &rng_state,
                                              &anomaly_injected,
                                              NULL),
            .sampling_frequency_hz = 0.0f,
            .source = SENSOR_SOURCE_VIRTUAL,
            .signal_profile = ctx->signal_profile,
            .anomaly_injected = anomaly_injected,
        };

        if (ctx->benchmark.samples_generated_in_stage == 0U) {
            ctx->benchmark.min_interval_us = 0.0f;
            ctx->benchmark.max_interval_us = 0.0f;
        } else {
            const float interval_us = (float)(now_us - last_sample_timestamp_us);
            if (ctx->benchmark.samples_generated_in_stage == 1U ||
                interval_us < ctx->benchmark.min_interval_us) {
                ctx->benchmark.min_interval_us = interval_us;
            }
            if (ctx->benchmark.samples_generated_in_stage == 1U ||
                interval_us > ctx->benchmark.max_interval_us) {
                ctx->benchmark.max_interval_us = interval_us;
            }
        }
        last_sample_timestamp_us = now_us;

        (void)sample;
        ctx->timing.samples_generated++;
        ctx->timing.last_sample_us = sample.timestamp_us;
        ctx->benchmark.samples_generated_in_stage++;

        if ((now_us - last_progress_report_us) >= (uint64_t)PROJECT_BENCHMARK_PROGRESS_REPORT_MS * 1000ULL) {
            ESP_LOGI(TAG,
                     "Raw benchmark progress | generated=%" PRIu32
                     " elapsed=%.2fs",
                     ctx->benchmark.samples_generated_in_stage,
                     (double)((float)elapsed_us / 1000000.0f));
            last_progress_report_us = now_us;
        }
    }

    benchmark_finish_raw_mode(ctx, benchmark_end_us);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PROJECT_SUPERVISOR_HEARTBEAT_MS));
    }
}

esp_err_t signal_input_init(project_context_t *ctx)
{
    if (ctx == NULL || ctx->system_events == NULL || ctx->sample_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xEventGroupSetBits(ctx->system_events, PROJECT_EVENT_SIGNAL_INPUT_READY);
    if (ctx->mode == PROJECT_MODE_VIRTUAL_SENSOR) {
        ESP_LOGI(TAG,
                 "Signal input module ready in %s mode at %.1f Hz using profile %s",
                 signal_input_mode_name(ctx->mode),
                 (double)ctx->adaptive.current_sampling_frequency_hz,
                 project_signal_profile_name(ctx->signal_profile));
    } else {
        ESP_LOGI(TAG,
                 "Signal input module ready in %s mode using profile %s",
                 signal_input_mode_name(ctx->mode),
                 project_signal_profile_name(ctx->signal_profile));
    }
    return ESP_OK;
}

void signal_input_task(void *pvParameters)
{
    project_context_t *ctx = (project_context_t *)pvParameters;

    if (ctx->mode == PROJECT_MODE_SAMPLING_BENCHMARK) {
        run_sampling_benchmark_mode(ctx);
        return;
    }

    if (ctx->mode == PROJECT_MODE_RAW_SAMPLING_BENCHMARK) {
        run_raw_sampling_benchmark_mode(ctx);
        return;
    }

    run_virtual_sensor_mode(ctx);
}
