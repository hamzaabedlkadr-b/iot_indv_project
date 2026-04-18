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
- prepare compact `LoRaWAN` payloads ready for campus `TTN` testing,
- record runtime timing and communication metrics.

## Local Configuration

The committed [`include/project_config.h`](./include/project_config.h) file now keeps only safe public defaults.

For real local runs:

1. copy [`include/project_config_local.example.h`](./include/project_config_local.example.h) to `include/project_config_local.h`
2. fill in your local WiFi and broker settings
3. keep `project_config_local.h` private; it is ignored by git

## Communication Modes

The firmware can be configured in [`include/project_config.h`](./include/project_config.h):

- `PROJECT_COMMUNICATION_MODE_MQTT_ONLY`
- `PROJECT_COMMUNICATION_MODE_LORAWAN_ONLY`
- `PROJECT_COMMUNICATION_MODE_BOTH`

`PROJECT_LORAWAN_ENABLE_RADIO_TX` is currently kept at `0` so the firmware prepares real uplink payloads without pretending that a board-specific `TTN` radio driver already exists in the repo.

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
- `components/comm_lorawan/` compact LoRaWAN payload preparation and stub-ready uplink queueing
- `components/metrics/` timing, counters, and experiment logging
- `components/app_display/` serial status output
- `test/` firmware-level tests and validation notes
