# ESP32 Adaptive Sampling IoT Project

This repository contains an `ESP32 + FreeRTOS` implementation of the individual IoT assignment: generate or read a signal, estimate its dominant frequency locally, adapt the sampling rate, compute a windowed aggregate, send the result to an edge server with `MQTT/WiFi`, and send the same aggregate over `LoRaWAN + TTN`.

The project is intentionally built around a `virtual sensor` so the required signal is repeatable, measurable, and easy to compare across runs. An optional real-sensor path can be added later, but the graded pipeline is the virtual-signal path.

## Current Status

- `MQTT/WiFi` validation is complete on the real Heltec board on the home network.
- The firmware supports three signal profiles: `clean_reference`, `noisy_reference`, and `anomaly_stress`.
- The board has already been visualized live with `BetterSerialPlotter`.
- The integrated main app now publishes compact aggregate payloads over `LoRaWAN/TTN` on real hardware near working gateway coverage, and the serial plus `TTN` screenshots are saved in the repo.
- `Energy` comparison is still open, and `secure MQTT` is now implemented but still needs a live TLS validation run.

## Where To Start

- Assignment brief: [`ASSIGNMENT_BRIEF.md`](./ASSIGNMENT_BRIEF.md)
- Requirements and implementation framing: [`PROJECT_REQUIREMENTS.md`](./PROJECT_REQUIREMENTS.md)
- Submission snapshot: [`docs/SUBMISSION_SNAPSHOT.md`](./docs/SUBMISSION_SNAPSHOT.md)
- Evidence map: [`docs/GRADING_EVIDENCE_MATRIX.md`](./docs/GRADING_EVIDENCE_MATRIX.md)
- Current detailed progress report: [`docs/CURRENT_PROGRESS_REPORT.md`](./docs/CURRENT_PROGRESS_REPORT.md)
- Historical detailed phase report: [`docs/PHASE1_REPORT.tex`](./docs/PHASE1_REPORT.tex) and [`docs/PHASE1_REPORT.pdf`](./docs/PHASE1_REPORT.pdf)
- Presentation strategy: [`docs/PRESENTATION_PLAYBOOK.md`](./docs/PRESENTATION_PLAYBOOK.md)
- Screenshot checklist: [`docs/EVIDENCE_SCREENSHOT_CHECKLIST.md`](./docs/EVIDENCE_SCREENSHOT_CHECKLIST.md)
- Runtime notes from the live board sessions: [`results/runtime_notes_2026-04-17.md`](./results/runtime_notes_2026-04-17.md)
- Latest clean MQTT evidence bundle: [`results/mqtt_evidence_2026-04-18.md`](./results/mqtt_evidence_2026-04-18.md)
- Latest LoRaWAN evidence bundle: [`results/lorawan_evidence_2026-04-20.md`](./results/lorawan_evidence_2026-04-20.md)

## System Pipeline

```text
virtual signal
    -> periodic sampling on ESP32
    -> FFT / dominant-frequency estimation
    -> adaptive sampling decision
    -> 5 s aggregate window
    -> MQTT over WiFi to local edge listener
    -> compact LoRaWAN payload for TTN
```

The final implementation path is the modular firmware under [`firmware/esp32_node/`](./firmware/esp32_node/). The rest of the repository exists to document, validate, and present that path.

## Rubric Summary

| Assignment item | Current result | Main evidence | Status |
| --- | --- | --- | --- |
| Maximum stable sampling frequency | Strict stable point measured at `50 Hz`; instability appears between `50 Hz` and `100 Hz` | [`docs/PHASE1_REPORT.tex`](./docs/PHASE1_REPORT.tex), [`docs/EVIDENCE_SCREENSHOT_CHECKLIST.md`](./docs/EVIDENCE_SCREENSHOT_CHECKLIST.md) | Measured |
| Optimal sampling frequency | Dominant frequency `5 Hz` leads to adaptive rate change `50 Hz -> 40 Hz` | [`docs/PHASE1_REPORT.tex`](./docs/PHASE1_REPORT.tex), [`pics/2026-04-18_better_serial_plotter_live_view.png`](./pics/2026-04-18_better_serial_plotter_live_view.png) | Validated |
| Aggregate over a window | `5 s` window average is computed and carried through MQTT / LoRa payloads | [`results/runtime_notes_2026-04-17.md`](./results/runtime_notes_2026-04-17.md) | Validated |
| MQTT edge delivery | Real board publishes to local broker and listener receives consecutive windows | [`results/mqtt_evidence_2026-04-18.md`](./results/mqtt_evidence_2026-04-18.md) | Validated |
| LoRaWAN / TTN cloud delivery | Integrated main-app LoRaWAN path validated on hardware; serial evidence shows `joined=1`, radio queuing, `Tx Done`, and `TTN` uplinks on `FPort 1` | [`results/lorawan_evidence_2026-04-20.md`](./results/lorawan_evidence_2026-04-20.md), [`cloud/ttn_payloads/README.md`](./cloud/ttn_payloads/README.md) | Validated |
| End-to-end latency | Clean home-network dataset captured from synchronized timestamps | [`results/summaries/mqtt_summary_2026-04-18_listener.md`](./results/summaries/mqtt_summary_2026-04-18_listener.md) | Validated |
| Communication volume | Adaptive payload size is measured; baseline-vs-adaptive comparison still needs the fixed-rate run | [`results/summaries/mqtt_summary_2026-04-18_listener.md`](./results/summaries/mqtt_summary_2026-04-18_listener.md) | Partial |
| Energy savings | Measurement method and run order are prepared; real meter runs still pending | [`docs/ENERGY_MEASUREMENT_RUNBOOK.md`](./docs/ENERGY_MEASUREMENT_RUNBOOK.md) | Pending |
| Secure MQTT | TLS-capable `MQTTS` support with certificate verification is implemented; live TLS evidence still pending | [`docs/SECURE_MQTT_SETUP.md`](./docs/SECURE_MQTT_SETUP.md) | Partial |
| Three input signals bonus | `clean`, `noisy`, and `anomaly` profiles were all validated on the real board over MQTT | [`results/summaries/`](./results/summaries/) | Validated |

## Signal Profiles

The virtual sensor supports three profiles that are already wired through the full aggregate pipeline:

- `clean_reference`: baseline sinusoidal input
- `noisy_reference`: same signal with Gaussian noise
- `anomaly_stress`: noisy signal with sparse injected spikes and non-zero `anomaly_count`

### Saved Profile Comparison

| Signal profile | Avg dominant frequency | Avg sampling frequency | Avg anomaly count | Avg payload size | Summary |
| --- | --- | --- | --- | --- | --- |
| `clean_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `446.0 B` | [`mqtt_summary_2026-04-17_clean_profile_synced.md`](./results/summaries/mqtt_summary_2026-04-17_clean_profile_synced.md) |
| `noisy_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `445.6 B` | [`mqtt_summary_2026-04-17_noisy_profile_synced.md`](./results/summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md) |
| `anomaly_stress` | `5.000 Hz` | `40.000 Hz` | `2.800` | `444.6 B` | [`mqtt_summary_2026-04-17_anomaly_profile_synced.md`](./results/summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md) |

The current anomaly profile keeps the dominant frequency stable while still producing non-zero spike counts, which is useful for the bonus discussion about robustness.

## Core Results

### 1. Maximum Sampling Frequency

The strict benchmark documented in [`docs/PHASE1_REPORT.tex`](./docs/PHASE1_REPORT.tex) found:

- `50 Hz` is the highest tested rate that passed the current strict stability rule
- `100 Hz` was very close but already showed one deadline miss
- `250 Hz` and above triggered watchdog-related instability in the benchmark

So the current project uses `50 Hz` as the clean oversampling baseline and adapts downward from there.

### 2. FFT And Adaptive Sampling

The input signal contains `3 Hz` and `5 Hz` sinusoidal components. The firmware tracks the dominant one and uses an `8x` oversampling policy rounded to practical steps:

- dominant frequency detected: `5.00 Hz`
- requested adaptive rate: `5.00 * 8 = 40.0 Hz`
- applied runtime change: `50.0 Hz -> 40.0 Hz`

This transition is shown in the saved phase report and in the live plotter screenshot:

- [`pics/2026-04-18_better_serial_plotter_live_view.png`](./pics/2026-04-18_better_serial_plotter_live_view.png)

### 3. Aggregate Computation

The aggregate is the average value over a `5 second` window. Each message also carries:

- `window_id`
- `sample_count`
- `sampling_frequency_hz`
- `dominant_frequency_hz`
- `average_value`
- `signal_profile`
- `anomaly_count`
- synchronized timing fields for latency analysis

This keeps the edge and cloud paths simple because both consumers receive the same summary object rather than raw samples.

### 4. MQTT Over WiFi

The freshest home-network evidence run is saved in:

- [`results/mqtt_evidence_2026-04-18.md`](./results/mqtt_evidence_2026-04-18.md)
- [`results/summaries/mqtt_summary_2026-04-18_listener.md`](./results/summaries/mqtt_summary_2026-04-18_listener.md)

Key numbers from that run:

| Metric | Value |
| --- | --- |
| Messages received | `5` |
| Missing windows | `0` |
| Avg listener latency | `1234421.600 us` |
| Avg end-to-end latency | `1587868.600 us` |
| Avg payload size | `454 B` |
| Signal profile | `clean_reference` |

### 5. LoRaWAN + TTN

The firmware now runs the Heltec `LoRaWAN` stack directly inside the main ESP-IDF application and records real radio activity in the runtime logs. The live validation on `2026-04-20` showed queued aggregate payloads, `joined=1` in the LoRaWAN heartbeat, and successful `Tx Done` events from the integrated main app.

Prepared artifacts:

- [`results/lorawan_evidence_2026-04-20.md`](./results/lorawan_evidence_2026-04-20.md)
- [`cloud/ttn_payloads/README.md`](./cloud/ttn_payloads/README.md)
- [`cloud/ttn_payloads/ttn_decoder.js`](./cloud/ttn_payloads/ttn_decoder.js)
- [`docs/RUNTIME_VALIDATION_CHECKLIST.md`](./docs/RUNTIME_VALIDATION_CHECKLIST.md)
- [`pics/2026-04-20_serial_lorawan_join_tx.png`](./pics/2026-04-20_serial_lorawan_join_tx.png)
- [`pics/2026-04-20_serial_lorawan_payload.png`](./pics/2026-04-20_serial_lorawan_payload.png)
- [`pics/2026-04-20_ttn_live_data_uplink.png`](./pics/2026-04-20_ttn_live_data_uplink.png)
- [`pics/2026-04-20_ttn_uplink_decoded.png`](./pics/2026-04-20_ttn_uplink_decoded.png)
- [`pics/2026-04-20_ttn_device_overview.png`](./pics/2026-04-20_ttn_device_overview.png)

### 6. Performance Evaluation

Current status by metric:

- `Latency`: measured and saved from synchronized timestamps.
- `Payload size`: measured for adaptive runs and the three signal profiles.
- `Energy`: planned carefully, but still needs real meter-based runs.
- `Execution time`: tracked in firmware timing counters and runtime notes, but still needs final presentation packaging.

One important observation: because the system already transmits one aggregate per window instead of raw samples, the network payload size does not change much with sampling frequency alone. The main expected benefit of adaptive sampling is lower local sensing and processing work, not a dramatic reduction in MQTT payload bytes per aggregate.

## BetterSerialPlotter Evidence

The project includes a simplified serial stream for `BetterSerialPlotter` so the live adaptive pipeline can be shown clearly without parsing verbose logs.

Saved screenshot:

- [`pics/2026-04-18_better_serial_plotter_live_view.png`](./pics/2026-04-18_better_serial_plotter_live_view.png)

This is presentation evidence for:

- current sampling frequency
- detected dominant frequency
- aggregate average stability

It is not used as evidence for MQTT delivery or TTN.

## Baseline Versus Adaptive Runs

To make the final energy comparison easier, the firmware now includes a dedicated config switch:

- `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U` for the normal adaptive mode
- `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U` for the fixed-rate baseline

That means baseline and adaptive runs can now be compared without removing the FFT stage or editing the task graph.

Use the detailed run order in:

- [`docs/ENERGY_MEASUREMENT_RUNBOOK.md`](./docs/ENERGY_MEASUREMENT_RUNBOOK.md)

## Quick Run Guide

### Firmware

Main firmware path:

- [`firmware/esp32_node/README.md`](./firmware/esp32_node/README.md)

Typical local workflow:

1. Copy `firmware/esp32_node/include/project_config_local.example.h` to `firmware/esp32_node/include/project_config_local.h` and set the local WiFi and broker values there.
2. Build and flash the board with PlatformIO.
3. Run the local MQTT listener on the laptop.
4. Save the `CSV`, `JSONL`, and markdown summary.

### Edge Listener

- [`edge_server/mqtt_listener/README.md`](./edge_server/mqtt_listener/README.md)

### Results And Analysis

- [`results/README.md`](./results/README.md)
- [`results/measurement_summary_template.md`](./results/measurement_summary_template.md)
- [`docs/SECURE_MQTT_SETUP.md`](./docs/SECURE_MQTT_SETUP.md)

## LLM Usage

The assignment requires documenting prompt usage and reflecting on the generated code. The repository already includes the structure for that:

- [`prompts/README.md`](./prompts/README.md)
- [`prompts/prompt_log_template.md`](./prompts/prompt_log_template.md)

The final submission still needs the prompt history to be curated into a cleaner final narrative.

## Remaining Work Before Submission

- run the meter-based `energy` comparison for baseline versus adaptive mode
- reuse the fixed-rate baseline run to finish the direct communication-volume comparison row
- validate the `secure MQTT` path against a TLS-capable broker and capture one saved run
- capture the remaining screenshots listed in [`docs/EVIDENCE_SCREENSHOT_CHECKLIST.md`](./docs/EVIDENCE_SCREENSHOT_CHECKLIST.md)

## Repository Guide

```text
firmware/esp32_node/        final ESP32 FreeRTOS implementation
edge_server/mqtt_listener/  local MQTT listener and logger
cloud/ttn_payloads/         TTN payload notes and decoder
docs/                       plans, checklists, and presentation material
results/                    captured logs, summaries, and runtime notes
prompts/                    LLM prompt log material for the final write-up
pics/                       saved workshop and README images
```
