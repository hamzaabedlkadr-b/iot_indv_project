# Submission Snapshot

Date: `2026-04-20`

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

- Maximum stable sampling frequency measured on the real Heltec board
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

## Strongest Evidence In The Repo

- Root repo overview: [`../../README.md`](../../README.md)
- Main technical walkthrough: [`../README.md`](../README.md)
- Detailed checkpoint report: [`CURRENT_PROGRESS_REPORT.md`](./CURRENT_PROGRESS_REPORT.md)
- Evidence matrix: [`GRADING_EVIDENCE_MATRIX.md`](./GRADING_EVIDENCE_MATRIX.md)
- MQTT evidence bundle: [`../results/mqtt_evidence_2026-04-18.md`](../results/mqtt_evidence_2026-04-18.md)
- LoRaWAN evidence bundle: [`../results/lorawan_evidence_2026-04-20.md`](../results/lorawan_evidence_2026-04-20.md)

## Current Honest Status By Area

| Area | Status | Notes |
| --- | --- | --- |
| Sampling benchmark | Complete | `50 Hz` is the highest tested clean stable point |
| Optimal sampling frequency | Complete | Dominant `5 Hz` component leads to runtime `40 Hz` |
| Aggregate computation | Complete | Per-window average is propagated through both communication paths |
| MQTT edge delivery | Complete | Validated on the real board on the home network |
| LoRaWAN / TTN delivery | Complete | Validated on real hardware with saved serial and `TTN` screenshots |
| Three signal profiles | Complete | Clean, noisy, and anomaly profiles were all exercised |
| Latency measurement | Complete | Saved summary artifacts exist |
| Communication volume | Partial | Current payload-size evidence exists; direct baseline-vs-adaptive comparison still needs the fixed-rate run |
| Energy measurement | Pending | Runbook and setup notes exist, but real meter values are still missing |
| Secure MQTT | Partial | TLS-capable firmware support exists, but live proof is still missing |
| Prompt-log curation | Pending | Repo structure exists, but final curated history is still missing |

## What Is Still Missing Before The Final Final Submission

- meter-based energy measurements
- one fixed-rate baseline run to complete the direct communication-volume comparison
- one live `MQTTS` validation run if a secure broker is available
- final curated prompt log

## Submission Position Today

As of `2026-04-20`, the project is already in a strong submission state:

- the full adaptive pipeline works on hardware,
- the edge path is proven with saved artifacts,
- the cloud path is proven with saved `TTN` screenshots,
- and the remaining work is concentrated in measurement and packaging rather than in missing core functionality.

If the project had to be submitted today, the correct description would be:

- core functionality: complete
- hardware validation: complete for `MQTT` and `LoRaWAN`
- final grading gaps: mainly `energy`, direct baseline comparison, and live `MQTTS`
