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

The security-related switches are also in [`include/project_config.h`](./include/project_config.h), and the setup notes are in:

- [`../docs/SECURE_MQTT_SETUP.md`](../../docs/SECURE_MQTT_SETUP.md)

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

## BetterSerialPlotter

The firmware can also emit a clean numeric serial stream for live plotting with BetterSerialPlotter.

See:

- [`../docs/BETTER_SERIAL_PLOTTER.md`](../../docs/BETTER_SERIAL_PLOTTER.md)

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
