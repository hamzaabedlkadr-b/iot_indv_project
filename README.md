# ESP32 Adaptive Sampling IoT System

Individual IoT assignment implementation using a `Heltec WiFi LoRa 32 V3`, `ESP-IDF`, and `FreeRTOS`.

This project builds an IoT node that generates a virtual sensor signal, samples it, analyzes it locally with an FFT, adapts the sampling frequency, computes a `5 s` aggregate, sends the aggregate to a nearby edge server over `MQTT/WiFi`, and sends the same aggregate to the cloud through `LoRaWAN + TTN`.

## Table Of Contents

- [Overview](#overview)
- [Assignment Coverage](#assignment-coverage)
- [System Architecture](#system-architecture)
- [Code Organization](#code-organization)
- [Implementation Details](#implementation-details)
- [Performance Evaluation](#performance-evaluation)
- [Evidence Gallery](#evidence-gallery)
- [Setup And Run](#setup-and-run)
- [Current Limitation](#current-limitation)

## Overview

The input signal follows the assignment model:

```text
s(t) = 2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)
```

![Clean reference input signal](./source/pics/input_signal_clean_reference_2026-04-22.png)

The firmware detects the `5 Hz` dominant component and adapts the sampling rate from the fixed baseline `50 Hz` to the optimized steady-state rate `40 Hz` using an `8x` oversampling policy.

Main features:

- FreeRTOS task pipeline for sampling, processing, control, communication, metrics, and display.
- Raw maximum sampling benchmark measured at `199,126.59 Hz`.
- FFT-based dominant-frequency detection.
- Adaptive sampling from `50 Hz` to `40 Hz`.
- Window average over `5 s`.
- MQTT edge delivery with saved listener logs and latency summaries.
- LoRaWAN + TTN uplink proof with serial and TTN screenshots.
- INA219 energy comparison for baseline, adaptive, and adaptive + deep sleep.
- Three signal profiles and extended anomaly-filter evaluation.

## Assignment Coverage

| Requirement | Result | Evidence |
| --- | --- | --- |
| Maximum sampling frequency | Raw benchmark: `199,126.59 Hz`; strict full-pipeline baseline: `50 Hz` | [`source/pics/Sampling_frequency.png`](./source/pics/Sampling_frequency.png), [`source/docs/CURRENT_PROGRESS_REPORT.md`](./source/docs/CURRENT_PROGRESS_REPORT.md) |
| Optimal sampling frequency | Dominant `5 Hz`, adaptive rate `40 Hz` | [`source/pics/2026-04-18_better_serial_plotter_live_view.png`](./source/pics/2026-04-18_better_serial_plotter_live_view.png) |
| Aggregate over window | `5 s` window average computed and propagated to MQTT and LoRaWAN | [`source/results/runtime_notes_2026-04-17.md`](./source/results/runtime_notes_2026-04-17.md) |
| MQTT over WiFi | Real Heltec board published aggregate messages to the edge listener | [`source/results/mqtt_evidence_2026-04-18.md`](./source/results/mqtt_evidence_2026-04-18.md), [`source/results/wifi_mqtt_evidence_2026-04-21.md`](./source/results/wifi_mqtt_evidence_2026-04-21.md) |
| LoRaWAN + TTN | Integrated main app joined and sent uplinks to TTN | [`source/results/lorawan_evidence_2026-04-20.md`](./source/results/lorawan_evidence_2026-04-20.md) |
| Energy saving | Adaptive awake run: `-0.06%`; optional deep sleep: `-26.04%` | [`source/results/summaries/ina219_comparison_2026-04-21.md`](./source/results/summaries/ina219_comparison_2026-04-21.md) |
| Communication volume | Represented samples drop `20%`; aggregate MQTT bytes stay flat | [`source/results/summaries/communication_volume_comparison_2026-04-21.md`](./source/results/summaries/communication_volume_comparison_2026-04-21.md) |
| End-to-end latency | Saved synchronized MQTT listener run | [`source/results/summaries/mqtt_summary_2026-04-18_listener.md`](./source/results/summaries/mqtt_summary_2026-04-18_listener.md) |
| Three input signals | `clean_reference`, `noisy_reference`, `anomaly_stress` | [`source/pics/input_signal_profiles_2026-04-22.png`](./source/pics/input_signal_profiles_2026-04-22.png), [`source/results/final_evidence_index_2026-04-21.md`](./source/results/final_evidence_index_2026-04-21.md) |
| Anomaly filters bonus | Z-score and Hampel evaluated at `p=1%, 5%, 10%` | [`source/results/summaries/anomaly_filter_evaluation_2026-04-21.md`](./source/results/summaries/anomaly_filter_evaluation_2026-04-21.md) |
| Secure MQTT | Firmware supports `mqtts://`; live TLS broker proof still pending | [`source/docs/SECURE_MQTT_SETUP.md`](./source/docs/SECURE_MQTT_SETUP.md) |

## System Architecture

```text
virtual signal
    -> sampling task
    -> FFT / dominant-frequency detection
    -> adaptive sampling controller
    -> 5 s aggregate window
    -> MQTT/WiFi edge server
    -> LoRaWAN/TTN cloud uplink
```

![System architecture overview](./source/pics/architecture_pipeline_overview.png)

Hardware used:

| Component | Purpose |
| --- | --- |
| `Heltec WiFi LoRa 32 V3` | Device under test running the main FreeRTOS firmware |
| `INA219` | Current and power measurement |
| second ESP32 or Heltec | INA219 monitor board |
| laptop | PlatformIO build, serial monitor, Mosquitto broker, MQTT listener |
| TTN console | LoRaWAN cloud validation |

## Code Organization

```text
source/
  firmware/esp32_node/        final ESP32 FreeRTOS firmware
  firmware/ina219_power_monitor/
                               second-board INA219 monitor firmware
  edge_server/mqtt_listener/  local MQTT listener and logger
  cloud/ttn_payloads/         TTN payload decoder and notes
  docs/                       reports, runbooks, setup notes
  results/                    saved CSV/JSON/Markdown measurements
  pics/                       screenshots and hardware photos
```

Most important files:

| Path | Purpose |
| --- | --- |
| [`source/firmware/esp32_node/main/app_main.c`](./source/firmware/esp32_node/main/app_main.c) | FreeRTOS task creation and supervision |
| [`source/firmware/esp32_node/components/signal_input/`](./source/firmware/esp32_node/components/signal_input/) | virtual signal generation and benchmarks |
| [`source/firmware/esp32_node/components/signal_processing/`](./source/firmware/esp32_node/components/signal_processing/) | FFT, windowing, aggregate generation |
| [`source/firmware/esp32_node/components/comm_mqtt/`](./source/firmware/esp32_node/components/comm_mqtt/) | WiFi and MQTT publishing |
| [`source/firmware/esp32_node/components/comm_lorawan/`](./source/firmware/esp32_node/components/comm_lorawan/) | compact LoRaWAN payload and Heltec radio integration |
| [`source/firmware/ina219_power_monitor/src/main.cpp`](./source/firmware/ina219_power_monitor/src/main.cpp) | INA219 monitor output |
| [`source/results/final_evidence_index_2026-04-21.md`](./source/results/final_evidence_index_2026-04-21.md) | map of evidence to assignment requirements |

## Implementation Details

### 1. Maximum Sampling Frequency

The project reports two frequencies because they answer different questions.

| Benchmark | Result | Meaning |
| --- | --- | --- |
| Raw class-style benchmark | `199,126.59 Hz` | maximum synthetic sample-generation throughput |
| Strict full-pipeline baseline | `50 Hz` | stable operating point with windows, FFT, queues, MQTT/LoRa paths, and supervision enabled |

The raw benchmark is the number used for comparison with the class reference repositories. The strict value is the safe baseline for the real adaptive pipeline.

### 2. FFT And Adaptive Sampling

The firmware computes the frequency spectrum for each window and detects the dominant component:

```text
dominant_frequency_hz = 5.00
adaptive_rate = 5.00 * 8 = 40.0 Hz
```

The board starts at `50 Hz`, then changes to `40 Hz` after the first window.

### 3. Aggregate Function

Every `5 s` window produces one aggregate object containing:

```text
window_id
sample_count
sampling_frequency_hz
dominant_frequency_hz
average_value
signal_profile
anomaly_count
timing fields
```

Both MQTT and LoRaWAN use this aggregate instead of sending raw samples.

### 4. MQTT Edge Server

The ESP32 publishes JSON aggregate messages to:

```text
project/adaptive-sampling-node/aggregate
```

The Python edge listener records:

- receive timestamp
- raw JSON payload
- parsed latency fields
- CSV and JSONL logs
- Markdown summary

### 5. LoRaWAN + TTN

The same aggregate is packed into a compact `10-byte` LoRaWAN payload and sent on `FPort 1`. The integrated main app was validated with a real TTN uplink and saved screenshots from the TTN console.

### 6. Signal Profiles

| Profile | Description |
| --- | --- |
| `clean_reference` | original `3 Hz + 5 Hz` signal |
| `noisy_reference` | signal plus Gaussian-like noise |
| `anomaly_stress` | noisy signal plus sparse injected spikes |

![Input signal profiles](./source/pics/input_signal_profiles_2026-04-22.png)

The anomaly evaluation also compares Z-score and Hampel filtering.

## Performance Evaluation

### Energy Consumption

The final INA219 measurement compares fixed baseline and adaptive mode using the same DUT, same monitor, same signal, same WiFi/MQTT workload, and same run duration.

| Metric | Baseline `50 Hz` | Adaptive `40 Hz` | Adaptive + deep sleep |
| --- | ---: | ---: | ---: |
| Average power | `553.0000 mW` | `552.6775 mW` | `410.8682 mW` |
| Integrated energy | `18.433238 mWh` | `18.422466 mWh` | `13.632451 mWh` |
| Delta vs baseline | reference | `-0.06%` | `-26.04%` |

![Energy comparison plot](./source/pics/result_energy_comparison_2026-04-22.png)

Interpretation:

- Adaptive sampling reduced the local sample-processing rate from `50 Hz` to `40 Hz`.
- Awake average power changed only slightly because WiFi, display, MQTT, and always-on FreeRTOS tasks dominate the board power.
- Deep sleep is a separate low-power strategy and produces the larger reduction.

### Communication Volume

| Mode | Sampling rate | Samples represented | MQTT messages | Total payload |
| --- | ---: | ---: | ---: | ---: |
| Fixed baseline | `50 Hz` | `1250` | `5` | `2270 B` |
| Adaptive | `40 Hz` | `1000` | `5` | `2270 B` |

![Communication volume plot](./source/pics/result_communication_volume_2026-04-22.png)

The represented local samples drop by `20%`, while MQTT bytes remain effectively constant because the system sends one aggregate per window.

### End-To-End Latency

The saved clean MQTT listener run recorded:

| Metric | Average |
| --- | ---: |
| listener latency | `1,234,421.6 us` |
| end-to-end latency | `1,587,868.6 us` |
| edge delay | `353,447.0 us` |

![MQTT latency plot](./source/pics/result_mqtt_latency_2026-04-22.png)

Evidence:

- [`source/results/summaries/mqtt_summary_2026-04-18_listener.md`](./source/results/summaries/mqtt_summary_2026-04-18_listener.md)

### Bonus Anomaly Filters

The anomaly-filter evaluation covers:

- anomaly rates `p=1%, 5%, 10%`
- Z-score and Hampel filters
- true positive rate
- false positive rate
- mean-error reduction
- FFT dominant-frequency impact
- execution time
- estimated filter energy
- Hampel window-size tradeoff

Main artifact:

- [`source/results/summaries/anomaly_filter_evaluation_2026-04-21.md`](./source/results/summaries/anomaly_filter_evaluation_2026-04-21.md)

![Anomaly filter plot](./source/pics/result_anomaly_filters_2026-04-22.png)

## Evidence Gallery

| Clean Input Signal | Three Signal Profiles |
| --- | --- |
| ![Clean reference input waveform generated from the assignment equation.](./source/pics/input_signal_clean_reference_2026-04-22.png) | ![Clean, noisy, and anomaly-stress input profiles used by the ESP32 firmware.](./source/pics/input_signal_profiles_2026-04-22.png) |
| `3 Hz + 5 Hz` assignment input. | Bonus profiles before FFT and aggregation. |

| Raw Sampling Benchmark | Hardware Power Setup | Adaptive Pipeline | TTN Live Uplink |
| --- | --- | --- | --- |
| ![Serial output showing the raw sampling benchmark result on the Heltec board.](./source/pics/Sampling_frequency.png) | ![INA219 and Heltec hardware setup used for energy measurement.](./source/pics/hardware.png) | ![BetterSerialPlotter view of the adaptive-sampling pipeline on the Heltec board.](./source/pics/2026-04-18_better_serial_plotter_live_view.png) | ![TTN Live Data showing fresh uplinks from the integrated main app.](./source/pics/2026-04-20_ttn_live_data_uplink.png) |
| `199,126.59 Hz` raw benchmark. | Two-board INA219 measurement setup. | Live sampling, frequency, and aggregate visualization. | Cloud-side proof of LoRaWAN uplinks. |

| Adaptive Power | Deep-Sleep Power | TTN Decoded Payload | TTN Device Overview |
| --- | --- | --- | --- |
| ![BetterSerialPlotter INA219 adaptive power trace.](./source/pics/2026-04-21_ina219_adaptive_betterserialplotter.png) | ![BetterSerialPlotter INA219 deep-sleep power trace.](./source/pics/2026-04-21_ina219_deepsleep_betterserialplotter.png) | ![TTN decoded uplink details showing payload metadata.](./source/pics/2026-04-20_ttn_uplink_decoded.png) | ![TTN device overview showing recent activity for the Heltec node.](./source/pics/2026-04-20_ttn_device_overview.png) |
| Adaptive awake power run. | Optional deep-sleep power run. | Decoded TTN uplink metadata. | Device activity after main-app integration. |

| Energy Result Plot | Communication Plot | MQTT Latency Plot | Anomaly Filter Plot |
| --- | --- | --- | --- |
| ![Static energy-result plot from INA219 summaries.](./source/pics/result_energy_comparison_2026-04-22.png) | ![Static communication-volume plot comparing fixed and adaptive modes.](./source/pics/result_communication_volume_2026-04-22.png) | ![Static MQTT latency plot from listener summaries.](./source/pics/result_mqtt_latency_2026-04-22.png) | ![Static anomaly-filter result plot comparing Z-score and Hampel filters.](./source/pics/result_anomaly_filters_2026-04-22.png) |
| Baseline vs adaptive vs deep sleep. | Local samples and aggregate bytes. | Average and p95 latency. | Detection and error reduction. |

## Setup And Run

### 1. Clone

```powershell
git clone https://github.com/hamzaabedlkadr-b/iot_indv_project.git
cd iot_indv_project
```

### 2. Configure Local Secrets

Copy the local config template:

```powershell
copy source\firmware\esp32_node\include\project_config_local.example.h source\firmware\esp32_node\include\project_config_local.h
```

Edit `project_config_local.h` with WiFi, MQTT, and TTN values. This file is ignored by git.

### 3. Build Firmware

```powershell
pio run -d source\firmware\esp32_node -e heltec_wifi_lora_32_V3
```

### 4. Upload Firmware

```powershell
pio run -d source\firmware\esp32_node -e heltec_wifi_lora_32_V3 -t upload
```

### 5. Run Edge Listener

```powershell
python -m pip install -r source\edge_server\mqtt_listener\requirements.txt
```

```powershell
python source\edge_server\mqtt_listener\listen_aggregates.py --host <BROKER_HOST> --port 1883 --topic project/adaptive-sampling-node/aggregate --csv source\results\summaries\latest_listener.csv --jsonl source\results\summaries\latest_listener.jsonl
```

### 6. Power Test Helper

For a second person rerunning the power test, use:

- [`source/docs/POWER_TEST_QUICKSTART_FOR_FRIEND.md`](./source/docs/POWER_TEST_QUICKSTART_FOR_FRIEND.md)

## Current Limitation

The only remaining weaker item is live secure-MQTT proof. The firmware supports `mqtts://`, certificate verification, and optional authentication, but the saved live broker evidence is still for local plain MQTT.

Setup note:

- [`source/docs/SECURE_MQTT_SETUP.md`](./source/docs/SECURE_MQTT_SETUP.md)

## Submission Notes

- Local secrets are kept out of git through `project_config_local.h`.
- Build outputs, PlatformIO cache, local logs, and scratch test folders are ignored.
- The main evidence map is [`source/results/final_evidence_index_2026-04-21.md`](./source/results/final_evidence_index_2026-04-21.md).
- The grading matrix is [`source/docs/GRADING_EVIDENCE_MATRIX.md`](./source/docs/GRADING_EVIDENCE_MATRIX.md).
