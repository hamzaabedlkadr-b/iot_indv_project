# Firmware

This folder contains the `ESP-IDF` firmware project for the ESP32 LoRa board.

Current implemented responsibilities:

- generate the assignment signal through the virtual sensor pipeline,
- support three virtual signal profiles for the bonus experiments,
- analyse each window to estimate the dominant frequency,
- adapt the sampling frequency based on that estimate,
- compute per-window aggregate results,
- fan out each aggregate to `MQTT`, `LoRaWAN`, or both,
- publish real aggregate payloads over `MQTT/WiFi`,
- serialize compact `LoRaWAN` payloads and deliver them through the integrated Heltec radio backend when local credentials are enabled,
- record runtime timing and communication metrics.

## Build Prerequisites

To build the integrated `LoRaWAN` path locally, install:

- `PlatformIO` with the `espressif32` platform
- `ESP32 Arduino core 3.3.7` under `%LOCALAPPDATA%\\Arduino15\\packages\\esp32\\hardware\\esp32\\3.3.7`
- `Heltec_ESP32_Dev-Boards` under `%USERPROFILE%\\Documents\\Arduino\\libraries\\Heltec_ESP32_Dev-Boards`, or set `HELTEC_ESP32_DEV_BOARDS_DIR`

This repo includes a small `components/arduino/` bridge so the local Arduino core can be linked from the `ESP-IDF` build, and it uses [`partitions_heltec_8mb.csv`](./partitions_heltec_8mb.csv) because the Heltec `LoRaWAN` runtime expects an OTA-style `8 MB` flash layout.

## Local Configuration

The committed [`include/project_config.h`](./include/project_config.h) file now keeps only safe public defaults.

For real local runs:

1. copy [`include/project_config_local.example.h`](./include/project_config_local.example.h) to `include/project_config_local.h`
2. fill in your local WiFi, broker, and `TTN` credential settings
3. keep `project_config_local.h` private; it is ignored by git

## Communication Modes

The firmware can be configured in [`include/project_config.h`](./include/project_config.h):

- `PROJECT_COMMUNICATION_MODE_MQTT_ONLY`
- `PROJECT_COMMUNICATION_MODE_LORAWAN_ONLY`
- `PROJECT_COMMUNICATION_MODE_BOTH`

The public config keeps `PROJECT_LORAWAN_ENABLE_RADIO_TX = 0U` so the repo does not ship active `TTN` credentials by default. The integrated Heltec radio path is implemented and already validated on `2026-04-20`; enable radio TX locally when your own `TTN` values are present in `project_config_local.h`.

## MQTT Security

The MQTT transport now supports both:

- plain `mqtt://` for local development
- verified `mqtts://` for secure-broker validation

The security-related switches are also in [`include/project_config.h`](./include/project_config.h), and the validated TLS evidence is summarized in:

- [`../../../README.md#4b-secure-mqtt-over-tls`](../../../README.md#4b-secure-mqtt-over-tls)

## Signal Profiles

The virtual signal can be switched in [`include/project_config.h`](./include/project_config.h):

- `PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE`
- `PROJECT_SIGNAL_PROFILE_NOISY_REFERENCE`
- `PROJECT_SIGNAL_PROFILE_ANOMALY_STRESS`

All three profiles keep the same base `3 Hz + 5 Hz` signal so the adaptive-sampling logic stays comparable between runs:

- `clean_reference`: deterministic baseline without noise
- `noisy_reference`: adds low Gaussian-like noise with `sigma=0.2`
- `anomaly_stress`: adds the same noise plus sparse large spikes for stress testing

The selected profile is carried into the aggregate logs and MQTT payloads as `signal_profile`, and anomaly-heavy windows also report `anomaly_count`.

## Baseline Versus Adaptive Mode

The sampling policy can now be switched in [`include/project_config.h`](./include/project_config.h):

- `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U` keeps the normal adaptive policy enabled
- `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U` holds the board at the fixed `50 Hz` baseline

This makes the later energy comparison much easier because the same firmware pipeline can be reused for both runs.

## Benchmark Modes

The firmware now supports two different benchmark styles:

- `PROJECT_ENABLE_SAMPLING_BENCHMARK = 1U`: strict full-pipeline benchmark for the highest clean stable operating rate
- `PROJECT_ENABLE_RAW_SAMPLING_BENCHMARK = 1U`: raw synthetic sample throughput benchmark, closer to the simpler class-repo "max frequency" style

Use the raw benchmark when you want a large theoretical throughput number. The latest saved raw result is `199,126.59 Hz`. Keep the strict benchmark when you want the more conservative full-pipeline result used for the `50 Hz -> 40 Hz` adaptive run.

## BetterSerialPlotter

The firmware can also emit a clean numeric serial stream for live plotting with BetterSerialPlotter.

See:

- [`../../../README.md#betterserialplotter-evidence`](../../../README.md#betterserialplotter-evidence)

## Internal Layout

- `main/` application entry point and supervisor logging
- `components/signal_input/` virtual sensor and benchmark input modes
- `components/signal_processing/` window buffering, spectral analysis, and aggregate creation
- `components/sampling_control/` adaptive sampling policy
- `components/comm_mqtt/` WiFi and MQTT publishing
- `components/comm_lorawan/` compact LoRaWAN payload packing and integrated Heltec radio delivery
- `components/metrics/` timing, counters, and experiment logging
- `components/app_display/` serial status output
- `test/` firmware-level tests and validation notes
