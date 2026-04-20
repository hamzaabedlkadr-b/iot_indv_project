#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

typedef enum {
    SENSOR_SOURCE_VIRTUAL = 0,
    SENSOR_SOURCE_HC_SR04 = 1
} sensor_source_t;

typedef enum {
    SIGNAL_PROFILE_CLEAN_REFERENCE = 0,
    SIGNAL_PROFILE_NOISY_REFERENCE = 1,
    SIGNAL_PROFILE_ANOMALY_STRESS = 2
} signal_profile_t;

static inline const char *project_signal_profile_name(signal_profile_t profile)
{
    switch (profile) {
        case SIGNAL_PROFILE_CLEAN_REFERENCE:
            return "clean_reference";
        case SIGNAL_PROFILE_NOISY_REFERENCE:
            return "noisy_reference";
        case SIGNAL_PROFILE_ANOMALY_STRESS:
            return "anomaly_stress";
        default:
            return "unknown";
    }
}

typedef enum {
    PROJECT_MODE_PHASE2_SKELETON = 0,
    PROJECT_MODE_VIRTUAL_SENSOR = 1,
    PROJECT_MODE_HC_SR04_DEMO = 2,
    PROJECT_MODE_SAMPLING_BENCHMARK = 3
} project_mode_t;

typedef enum {
    PROJECT_COMM_MODE_MQTT_ONLY = 0,
    PROJECT_COMM_MODE_LORAWAN_ONLY = 1,
    PROJECT_COMM_MODE_BOTH = 2
} project_comm_mode_t;

static inline uint8_t project_comm_mode_supports_mqtt(project_comm_mode_t mode)
{
    return mode == PROJECT_COMM_MODE_MQTT_ONLY || mode == PROJECT_COMM_MODE_BOTH;
}

static inline uint8_t project_comm_mode_supports_lorawan(project_comm_mode_t mode)
{
    return mode == PROJECT_COMM_MODE_LORAWAN_ONLY || mode == PROJECT_COMM_MODE_BOTH;
}

typedef struct {
    uint32_t sample_id;
    uint64_t timestamp_us;
    float value;
    float sampling_frequency_hz;
    sensor_source_t source;
    signal_profile_t signal_profile;
    uint8_t anomaly_injected;
} raw_sample_t;

typedef struct {
    uint32_t window_id;
    uint64_t window_start_us;
    uint64_t window_end_us;
    float dominant_frequency_hz;
    float peak_magnitude;
} fft_result_t;

typedef struct {
    uint32_t window_id;
    uint64_t window_start_us;
    uint64_t window_end_us;
    uint32_t sample_count;
    float sampling_frequency_hz;
    float dominant_frequency_hz;
    float average_value;
    signal_profile_t signal_profile;
    uint32_t anomaly_count;
} aggregate_result_t;

typedef struct {
    uint64_t created_at_us;
    uint32_t window_id;
    uint64_t source_timestamp_us;
    uint64_t publish_latency_us;
    uint8_t port;
    uint8_t binary_payload_size;
    uint8_t binary_payload[16];
    char topic[64];
    char payload[512];
} transmission_payload_t;

typedef struct {
    uint64_t boot_time_us;
    uint64_t last_sample_us;
    uint64_t last_fft_us;
    uint64_t last_aggregate_us;
    uint64_t last_publish_us;
    uint32_t samples_generated;
    uint32_t windows_processed;
    uint32_t aggregates_processed;
    uint32_t mqtt_messages_prepared;
    uint32_t mqtt_messages_sent;
    uint32_t lora_messages_prepared;
    uint32_t lora_messages_sent;
    uint64_t last_edge_latency_us;
} timing_metrics_t;

typedef struct {
    uint32_t stage_index;
    uint32_t total_stages;
    uint32_t samples_generated_in_stage;
    uint32_t samples_consumed_in_stage;
    uint32_t queue_drops_in_stage;
    uint32_t deadline_misses_in_stage;
    uint32_t stage_completed;
    float target_frequency_hz;
    float achieved_frequency_hz;
    float min_interval_us;
    float max_interval_us;
    int32_t worst_lateness_us;
    uint64_t stage_start_us;
    uint64_t last_stage_duration_us;
    uint8_t benchmark_complete;
    uint8_t last_stage_stable;
} sampling_benchmark_metrics_t;

typedef struct {
    float current_sampling_frequency_hz;
    float last_requested_sampling_frequency_hz;
    float last_dominant_frequency_hz;
    float last_peak_magnitude;
    uint32_t last_window_id;
    uint32_t frequency_updates;
    uint64_t last_update_us;
} adaptive_sampling_state_t;

typedef struct {
    QueueHandle_t sample_queue;
    QueueHandle_t fft_queue;
    QueueHandle_t aggregate_mqtt_queue;
    QueueHandle_t aggregate_lorawan_queue;
    QueueHandle_t mqtt_queue;
    QueueHandle_t lorawan_queue;
    EventGroupHandle_t system_events;
    timing_metrics_t timing;
    sampling_benchmark_metrics_t benchmark;
    adaptive_sampling_state_t adaptive;
    aggregate_result_t latest_aggregate;
    uint8_t latest_aggregate_valid;
    signal_profile_t signal_profile;
    project_comm_mode_t communication_mode;
    project_mode_t mode;
} project_context_t;

#endif
