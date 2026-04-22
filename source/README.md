# ESP32 Adaptive Sampling IoT Project

This repository contains an `ESP32 + FreeRTOS` implementation of the individual IoT assignment: generate or read a signal, estimate its dominant frequency locally, adapt the sampling rate, compute a windowed aggregate, send the result to an edge server with `MQTT/WiFi`, and send the same aggregate over `LoRaWAN + TTN`.

The project is intentionally built around a `virtual sensor` so the required signal is repeatable, measurable, and easy to compare across runs. An optional real-sensor path can be added later, but the graded pipeline is the virtual-signal path.

![Clean reference input signal](./pics/input_signal_clean_reference_2026-04-22.png)

## Current Status

- `MQTT/WiFi` validation is complete on the real Heltec board on the home network.
- The firmware supports three signal profiles: `clean_reference`, `noisy_reference`, and `anomaly_stress`.
- The board has already been visualized live with `BetterSerialPlotter`.
- The integrated main app now publishes compact aggregate payloads over `LoRaWAN/TTN` on real hardware near working gateway coverage, and the serial plus `TTN` screenshots are saved in the repo.
- `Energy` comparison is measured with the `INA219` two-board setup, and `secure MQTT` is validated with a live TLS broker run.

## Where To Start

- Assignment brief: [`ASSIGNMENT_BRIEF.md`](./ASSIGNMENT_BRIEF.md)
- Requirements and implementation framing: [`PROJECT_REQUIREMENTS.md`](./PROJECT_REQUIREMENTS.md)
- Final evidence index: [`results/final_evidence_index_2026-04-21.md`](./results/final_evidence_index_2026-04-21.md)
- Runtime notes from the live board sessions: [`results/runtime_notes_2026-04-17.md`](./results/runtime_notes_2026-04-17.md)
- Latest clean MQTT evidence bundle: [`results/mqtt_evidence_2026-04-18.md`](./results/mqtt_evidence_2026-04-18.md)
- Fresh WiFi/MQTT evidence bundle after the network change: [`results/wifi_mqtt_evidence_2026-04-21.md`](./results/wifi_mqtt_evidence_2026-04-21.md)
- Secure MQTT evidence bundle: [`results/secure_mqtt_evidence_2026-04-22.md`](./results/secure_mqtt_evidence_2026-04-22.md)
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
| Maximum sampling frequency | Raw class-style benchmark measured `199,126.59 Hz`; strict full-pipeline clean operating point remains `50 Hz` | [`pics/Sampling_frequency.png`](./pics/Sampling_frequency.png), [`results/final_evidence_index_2026-04-21.md`](./results/final_evidence_index_2026-04-21.md) | Measured |
| Optimal sampling frequency | Dominant frequency `5 Hz` leads to adaptive rate change `50 Hz -> 40 Hz` | [`pics/2026-04-18_better_serial_plotter_live_view.png`](./pics/2026-04-18_better_serial_plotter_live_view.png) | Validated |
| Aggregate over a window | `5 s` window average is computed and carried through MQTT / LoRa payloads | [`results/runtime_notes_2026-04-17.md`](./results/runtime_notes_2026-04-17.md) | Validated |
| MQTT edge delivery | Real board publishes to local broker and listener receives consecutive windows | [`results/mqtt_evidence_2026-04-18.md`](./results/mqtt_evidence_2026-04-18.md) | Validated |
| LoRaWAN / TTN cloud delivery | Integrated main-app LoRaWAN path validated on hardware; serial evidence shows `joined=1`, radio queuing, `Tx Done`, and `TTN` uplinks on `FPort 1` | [`results/lorawan_evidence_2026-04-20.md`](./results/lorawan_evidence_2026-04-20.md), [`cloud/ttn_payloads/README.md`](./cloud/ttn_payloads/README.md) | Validated |
| End-to-end latency | Clean home-network dataset captured from synchronized timestamps | [`results/summaries/mqtt_summary_2026-04-18_listener.md`](./results/summaries/mqtt_summary_2026-04-18_listener.md) | Validated |
| Communication volume | Baseline-vs-adaptive comparison is complete: local represented samples drop `20%`, while MQTT aggregate bytes stay effectively constant | [`results/summaries/communication_volume_comparison_2026-04-21.md`](./results/summaries/communication_volume_comparison_2026-04-21.md) | Validated |
| Energy savings | `INA219` baseline/adaptive runs show a small awake-mode saving (`-0.06%`), while optional deep sleep reduces energy by `-26.04%` | [`results/summaries/ina219_comparison_2026-04-21.md`](./results/summaries/ina219_comparison_2026-04-21.md), [`results/summaries/ina219_three_mode_comparison_2026-04-21.md`](./results/summaries/ina219_three_mode_comparison_2026-04-21.md), [`pics/hardware.png`](./pics/hardware.png) | Measured |
| Secure MQTT | Live `MQTTS` run validated on `broker.emqx.io:8883`; listener required TLS verification and the ESP32 log showed certificate validation | [`results/secure_mqtt_evidence_2026-04-22.md`](./results/secure_mqtt_evidence_2026-04-22.md), [`results/summaries/secure_mqtt_summary_final_2026-04-22.md`](./results/summaries/secure_mqtt_summary_final_2026-04-22.md) | Validated |
| Three input signals bonus | `clean`, `noisy`, and `anomaly` profiles were all validated on the real board over MQTT | [`pics/input_signal_profiles_2026-04-22.png`](./pics/input_signal_profiles_2026-04-22.png), [`results/final_evidence_index_2026-04-21.md`](./results/final_evidence_index_2026-04-21.md), [`results/summaries/signal_profile_comparison_2026-04-18.txt`](./results/summaries/signal_profile_comparison_2026-04-18.txt) | Validated |
| Anomaly filter bonus | `Z-score` and `Hampel` filters evaluated at `p=1%, 5%, 10%`, including `TPR`, `FPR`, mean-error reduction, FFT impact, execution time, estimated filter energy, and window-size tradeoff | [`results/summaries/anomaly_filter_evaluation_2026-04-21.md`](./results/summaries/anomaly_filter_evaluation_2026-04-21.md) | Validated |

## Signal Profiles

The virtual sensor supports three profiles that are already wired through the full aggregate pipeline:

- `clean_reference`: baseline sinusoidal input
- `noisy_reference`: same signal with Gaussian noise
- `anomaly_stress`: noisy signal with sparse injected spikes and non-zero `anomaly_count`

![Input signal profiles](./pics/input_signal_profiles_2026-04-22.png)

### Saved Profile Comparison

| Signal profile | Avg dominant frequency | Avg sampling frequency | Avg anomaly count | Avg payload size | Summary |
| --- | --- | --- | --- | --- | --- |
| `clean_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `446.0 B` | [`mqtt_summary_2026-04-17_clean_profile_synced.md`](./results/summaries/mqtt_summary_2026-04-17_clean_profile_synced.md) |
| `noisy_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `445.6 B` | [`mqtt_summary_2026-04-17_noisy_profile_synced.md`](./results/summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md) |
| `anomaly_stress` | `5.000 Hz` | `40.000 Hz` | `2.800` | `444.6 B` | [`mqtt_summary_2026-04-17_anomaly_profile_synced.md`](./results/summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md) |

The current anomaly profile keeps the dominant frequency stable while still producing non-zero spike counts, which is useful for the bonus discussion about robustness.

### Anomaly Filter Evaluation

The extended anomaly bonus is covered by a deterministic evaluation of `Z-score` and `Hampel` filters using the same synthetic signal model as the firmware. It evaluates anomaly probabilities `p=1%, 5%, 10%` and reports:

- true positive rate and false positive rate
- mean-error reduction after filtering
- FFT dominant-frequency error before and after filtering
- resulting adaptive sampling frequency
- filter execution time and estimated active-energy impact
- Hampel window-size tradeoff for delay and memory

Main artifact:

- [`results/summaries/anomaly_filter_evaluation_2026-04-21.md`](./results/summaries/anomaly_filter_evaluation_2026-04-21.md)

## Core Results

The final static result plots are indexed in [`results/summaries/result_plot_index_2026-04-22.md`](./results/summaries/result_plot_index_2026-04-22.md).

### 1. Maximum Sampling Frequency

The latest class-style raw benchmark measured the sample-generation throughput without the full MQTT/LoRa/display pipeline:

```text
Raw benchmark result | generated=199126 min_dt=3.0us max_dt=53.0us achieved=199126.59Hz stable=yes duration=1.00s
```

Evidence:

- [`pics/Sampling_frequency.png`](./pics/Sampling_frequency.png)

This is the number to present when comparing with the class example repositories that report raw or simple-loop maximum sampling rates.

The older strict full-pipeline benchmark remains useful because it tests the complete application with windows, FFT, queues, aggregate creation, and runtime supervision. Under that stricter rule:

- `50 Hz` is the highest tested clean operating point
- `100 Hz` was very close but already showed one deadline miss
- `250 Hz` and above triggered watchdog-related instability

So the project reports `199,126.59 Hz` as the raw benchmark result, while still using `50 Hz` as the conservative full-pipeline baseline that adapts down to `40 Hz`.

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

### 4b. Secure MQTT Over TLS

The secure MQTT path was validated with the same aggregate payload format over `MQTTS`.

| Metric | Value |
| --- | --- |
| Broker | `broker.emqx.io:8883` |
| Topic | `iot_indv_project/hamza/adaptive-sampling-node/secure` |
| TLS listener mode | `tls=enabled verify=required` |
| ESP32 certificate status | `Certificate validated` |
| Messages received | `3` |
| Missing windows | `0` |
| Average end-to-end latency | `907,561.667 us` |

Evidence:

- [`results/secure_mqtt_evidence_2026-04-22.md`](./results/secure_mqtt_evidence_2026-04-22.md)
- [`results/summaries/secure_mqtt_summary_final_2026-04-22.md`](./results/summaries/secure_mqtt_summary_final_2026-04-22.md)

### 5. LoRaWAN + TTN

The firmware now runs the Heltec `LoRaWAN` stack directly inside the main ESP-IDF application and records real radio activity in the runtime logs. The live validation on `2026-04-20` showed queued aggregate payloads, `joined=1` in the LoRaWAN heartbeat, and successful `Tx Done` events from the integrated main app.

Prepared artifacts:

- [`results/lorawan_evidence_2026-04-20.md`](./results/lorawan_evidence_2026-04-20.md)
- [`cloud/ttn_payloads/README.md`](./cloud/ttn_payloads/README.md)
- [`cloud/ttn_payloads/ttn_decoder.js`](./cloud/ttn_payloads/ttn_decoder.js)
- [`results/final_evidence_index_2026-04-21.md`](./results/final_evidence_index_2026-04-21.md)
- [`pics/2026-04-20_serial_lorawan_join_tx.png`](./pics/2026-04-20_serial_lorawan_join_tx.png)
- [`pics/2026-04-20_serial_lorawan_payload.png`](./pics/2026-04-20_serial_lorawan_payload.png)
- [`pics/2026-04-20_ttn_live_data_uplink.png`](./pics/2026-04-20_ttn_live_data_uplink.png)
- [`pics/2026-04-20_ttn_uplink_decoded.png`](./pics/2026-04-20_ttn_uplink_decoded.png)
- [`pics/2026-04-20_ttn_device_overview.png`](./pics/2026-04-20_ttn_device_overview.png)

### 6. Performance Evaluation

Current status by metric:

- `Latency`: measured and saved from synchronized timestamps.
- `Payload size`: measured for adaptive runs and compared against the fixed-rate baseline using the same aggregate-per-window protocol.
- `Energy`: measured with the `INA219` monitor for fixed `50 Hz` and adaptive `40 Hz` runs.
- `Execution time`: tracked through firmware timing fields, edge-delay metrics, and the anomaly-filter execution-time table.

One important observation: because the system already transmits one aggregate per window instead of raw samples, the network payload size does not change much with sampling frequency alone. The saved communication-volume table shows `1250` represented samples for the fixed `50 Hz` baseline versus `1000` represented samples for adaptive `40 Hz` over the same five windows (`-20%` local sample work), while both modes send five compact aggregate MQTT messages.

Evidence:

- [`results/summaries/communication_volume_comparison_2026-04-21.md`](./results/summaries/communication_volume_comparison_2026-04-21.md)

### 7. Energy Measurement

The final `INA219` comparison used the same DUT, monitor board, WiFi network, `MQTT_ONLY` mode, and `clean_reference` signal for both runs:

| Metric | Baseline `50 Hz` | Adaptive `40 Hz` | Delta |
| --- | --- | --- | --- |
| Average current | `110.4953 mA` | `110.6984 mA` | `+0.18%` |
| Average power | `553.0000 mW` | `552.6775 mW` | `-0.06%` |
| Integrated energy | `18.433238 mWh` | `18.422466 mWh` | `-0.06%` |
| Peak power | `873.0000 mW` | `793.0000 mW` | `-9.16%` |

This result is intentionally reported honestly: adaptive sampling reduced the sample-processing rate, but the board stayed awake with WiFi, display, FreeRTOS tasks, and MQTT activity, so average electrical energy changed only slightly. This is consistent with the instructor-shared reference projects where WiFi and always-on runtime work dominate the power profile.

Evidence:

- [`results/summaries/ina219_baseline_2026-04-21.md`](./results/summaries/ina219_baseline_2026-04-21.md)
- [`results/summaries/ina219_adaptive_2026-04-21.md`](./results/summaries/ina219_adaptive_2026-04-21.md)
- [`results/summaries/ina219_comparison_2026-04-21.md`](./results/summaries/ina219_comparison_2026-04-21.md)
- [`results/summaries/ina219_deepsleep_2026-04-21.md`](./results/summaries/ina219_deepsleep_2026-04-21.md)
- [`results/summaries/ina219_three_mode_comparison_2026-04-21.md`](./results/summaries/ina219_three_mode_comparison_2026-04-21.md)
- [`pics/hardware.png`](./pics/hardware.png)
- [`pics/2026-04-21_ina219_adaptive_betterserialplotter.png`](./pics/2026-04-21_ina219_adaptive_betterserialplotter.png)
- [`pics/2026-04-21_ina219_deepsleep_betterserialplotter.png`](./pics/2026-04-21_ina219_deepsleep_betterserialplotter.png)

An additional deep-sleep experiment reduced integrated energy to `13.632451 mWh` over `119.480 s`, about `26.04%` lower than the fixed `50 Hz` awake baseline. This is reported as a separate duty-cycle optimization, not as the main adaptive-sampling comparison.

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

- [`firmware/ina219_power_monitor/README.md`](./firmware/ina219_power_monitor/README.md)

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
- [`results/secure_mqtt_evidence_2026-04-22.md`](./results/secure_mqtt_evidence_2026-04-22.md)

## Presentation Checklist

- Explain raw max sampling (`199,126.59 Hz`) versus stable full-pipeline baseline (`50 Hz`).
- Show FFT detecting `5 Hz` and the adaptive change to `40 Hz`.
- Show MQTT, secure MQTT, and LoRaWAN evidence separately so each communication requirement is clear.
- Show energy honestly: awake adaptive savings are small (`-0.06%`), while optional deep sleep gives a larger reduction (`-26.04%`).
- Show the three signal profiles and anomaly-filter plot for the bonus points.

## Repository Guide

```text
firmware/esp32_node/        final ESP32 FreeRTOS implementation
edge_server/mqtt_listener/  local MQTT listener and logger
cloud/ttn_payloads/         TTN payload notes and decoder
results/                    captured logs, summaries, and runtime notes
pics/                       saved workshop and README images
```
