# Submission Snapshot

Date: `2026-04-21`

This note is the short submission-oriented summary of the project state as it stands today.

It is meant to complement:

- the full technical walkthrough in [`../README.md`](../README.md),
- the detailed checkpoint report in [`CURRENT_PROGRESS_REPORT.md`](./CURRENT_PROGRESS_REPORT.md),
- and the rubric-to-evidence map in [`GRADING_EVIDENCE_MATRIX.md`](./GRADING_EVIDENCE_MATRIX.md).

## Project Summary

The project implements the required `ESP32 + FreeRTOS` adaptive-sampling pipeline:

```text
virtual signal
    -> sampling
    -> FFT / dominant-frequency estimation
    -> adaptive sampling update
    -> 5 s aggregate
    -> MQTT over WiFi to local edge listener
    -> compact LoRaWAN payload over TTN
```

The signal used in the firmware follows the assignment requirement:

```text
2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)
```

The dominant component is `5 Hz`, and the runtime adaptive policy reduces the sampling rate from `50 Hz` to `40 Hz`.

## What Is Already Validated

- Maximum sampling frequency measured on the real Heltec board: raw benchmark `199,126.59 Hz`, with `50 Hz` retained as the strict full-pipeline baseline
- FFT-based dominant-frequency estimation
- Adaptive sampling update from `50 Hz` to `40 Hz`
- Correct `5 s` aggregate computation
- Real `MQTT/WiFi` delivery to the local edge listener
- Real `LoRaWAN + TTN` delivery from the integrated main application
- Three input signal profiles:
  - `clean_reference`
  - `noisy_reference`
  - `anomaly_stress`
- Saved latency and payload-size evidence from real runs
- Saved baseline-vs-adaptive communication-volume comparison
- Saved Z-score and Hampel anomaly-filter evaluation for `p=1%, 5%, 10%`
- INA219 energy comparison for fixed `50 Hz` versus adaptive `40 Hz`

## Strongest Evidence In The Repo

- Root repo overview: [`../../README.md`](../../README.md)
- Main technical walkthrough: [`../README.md`](../README.md)
- Detailed checkpoint report: [`CURRENT_PROGRESS_REPORT.md`](./CURRENT_PROGRESS_REPORT.md)
- Evidence matrix: [`GRADING_EVIDENCE_MATRIX.md`](./GRADING_EVIDENCE_MATRIX.md)
- Final evidence index: [`../results/final_evidence_index_2026-04-21.md`](../results/final_evidence_index_2026-04-21.md)
- Raw sampling benchmark screenshot: [`../pics/Sampling_frequency.png`](../pics/Sampling_frequency.png)
- MQTT evidence bundle: [`../results/mqtt_evidence_2026-04-18.md`](../results/mqtt_evidence_2026-04-18.md)
- Fresh WiFi/MQTT evidence bundle: [`../results/wifi_mqtt_evidence_2026-04-21.md`](../results/wifi_mqtt_evidence_2026-04-21.md)
- LoRaWAN evidence bundle: [`../results/lorawan_evidence_2026-04-20.md`](../results/lorawan_evidence_2026-04-20.md)
- Energy comparison: [`../results/summaries/ina219_comparison_2026-04-21.md`](../results/summaries/ina219_comparison_2026-04-21.md)
- Three-mode energy comparison: [`../results/summaries/ina219_three_mode_comparison_2026-04-21.md`](../results/summaries/ina219_three_mode_comparison_2026-04-21.md)
- Communication-volume comparison: [`../results/summaries/communication_volume_comparison_2026-04-21.md`](../results/summaries/communication_volume_comparison_2026-04-21.md)
- Anomaly-filter evaluation: [`../results/summaries/anomaly_filter_evaluation_2026-04-21.md`](../results/summaries/anomaly_filter_evaluation_2026-04-21.md)
- Hardware setup photo: [`../pics/hardware.png`](../pics/hardware.png)

## Current Honest Status By Area

| Area | Status | Notes |
| --- | --- | --- |
| Sampling benchmark | Complete | Raw class-style benchmark measured `199,126.59 Hz`; strict full-pipeline clean point is `50 Hz` |
| Optimal sampling frequency | Complete | Dominant `5 Hz` component leads to runtime `40 Hz` |
| Aggregate computation | Complete | Per-window average is propagated through both communication paths |
| MQTT edge delivery | Complete | Validated on the real board on the home network |
| LoRaWAN / TTN delivery | Complete | Validated on real hardware with saved serial and `TTN` screenshots |
| Three signal profiles | Complete | Clean, noisy, and anomaly profiles were all exercised |
| Latency measurement | Complete | Saved summary artifacts exist |
| Communication volume | Complete | Fixed `50 Hz` versus adaptive `40 Hz` comparison shows represented samples drop `20%`, while aggregate MQTT bytes stay effectively constant |
| Energy measurement | Complete | `INA219` comparison shows adaptive `18.422466 mWh` vs baseline `18.433238 mWh` over `120 s`; optional deep sleep saves about `26.04%` |
| Secure MQTT | Partial | TLS-capable firmware support exists, but live proof is still missing |
| Anomaly filters | Complete | `Z-score` and `Hampel` filters are evaluated at `p=1%, 5%, 10%`, including `TPR`, `FPR`, FFT impact, execution time, estimated energy, and Hampel window-size tradeoff |

## What Is Still Missing Before The Final Final Submission

- one live `MQTTS` validation run if a secure broker is available

## Submission Position Today

As of `2026-04-21`, the project is already in a strong submission state:

- the full adaptive pipeline works on hardware,
- the edge path is proven with saved artifacts,
- the cloud path is proven with saved `TTN` screenshots,
- and the remaining work is concentrated in optional secure-MQTT proof rather than in missing core functionality.

If the project had to be submitted today, the correct description would be:

- core functionality: complete
- hardware validation: complete for `MQTT` and `LoRaWAN`
- final grading gap: mainly live `MQTTS` proof if a TLS-capable broker is available
