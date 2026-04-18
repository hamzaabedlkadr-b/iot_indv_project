# ESP32 Adaptive Sampling IoT Assignment

This repository contains an `ESP32 + FreeRTOS` implementation of the individual IoT assignment: generate a virtual sensor signal, estimate its dominant frequency with an `FFT`, adapt the sampling rate, compute a windowed aggregate, publish that aggregate to a nearby edge server over `MQTT/WiFi`, and prepare the same aggregate for `LoRaWAN + TTN`.

The technical project files live under [`source/`](./source/). The repository root is intentionally kept lightweight so the public submission looks clean and focused.

## Status Snapshot

| Assignment area | Current status | Main evidence |
| --- | --- | --- |
| Maximum stable sampling frequency | Validated on real hardware | [`source/docs/CURRENT_PROGRESS_REPORT.md`](./source/docs/CURRENT_PROGRESS_REPORT.md) |
| FFT-based adaptive sampling | Validated (`50 Hz -> 40 Hz`) | [`source/README.md`](./source/README.md) |
| Aggregate over a `5 s` window | Validated | [`source/README.md`](./source/README.md) |
| MQTT over WiFi to edge server | Validated on real hardware | [`source/results/mqtt_evidence_2026-04-18.md`](./source/results/mqtt_evidence_2026-04-18.md) |
| Three input signals bonus | Validated | [`source/results/summaries/`](./source/results/summaries/) |
| LoRaWAN + TTN cloud path | Payload path ready, live campus uplink still pending | [`source/cloud/ttn_payloads/README.md`](./source/cloud/ttn_payloads/README.md) |
| Energy comparison | Runbook prepared, meter-based measurements still pending | [`source/docs/ENERGY_MEASUREMENT_RUNBOOK.md`](./source/docs/ENERGY_MEASUREMENT_RUNBOOK.md) |
| Secure MQTT | TLS-capable firmware path implemented, live proof still pending | [`source/docs/SECURE_MQTT_SETUP.md`](./source/docs/SECURE_MQTT_SETUP.md) |

## Start Here

- Main technical walkthrough: [`source/README.md`](./source/README.md)
- Assignment requirements and project framing: [`source/PROJECT_REQUIREMENTS.md`](./source/PROJECT_REQUIREMENTS.md)
- Clean assignment brief summary: [`source/ASSIGNMENT_BRIEF.md`](./source/ASSIGNMENT_BRIEF.md)
- Evidence map: [`source/docs/GRADING_EVIDENCE_MATRIX.md`](./source/docs/GRADING_EVIDENCE_MATRIX.md)
- Current progress report: [`source/docs/CURRENT_PROGRESS_REPORT.md`](./source/docs/CURRENT_PROGRESS_REPORT.md)
- Firmware guide: [`source/firmware/esp32_node/README.md`](./source/firmware/esp32_node/README.md)
- Edge listener guide: [`source/edge_server/mqtt_listener/README.md`](./source/edge_server/mqtt_listener/README.md)

## Repository Layout

```text
source/
  firmware/esp32_node/        ESP32 FreeRTOS firmware
  edge_server/mqtt_listener/  local MQTT listener and logger
  cloud/ttn_payloads/         TTN decoder and cloud notes
  docs/                       runbooks, evidence matrix, reports
  results/                    saved measurements and summaries
  prompts/                    LLM prompt-log material
  pics/                       screenshots used in the write-up
```

## Quick Run

### 1. Configure local credentials without committing them

Copy:

- [`source/firmware/esp32_node/include/project_config_local.example.h`](./source/firmware/esp32_node/include/project_config_local.example.h)

to:

- `source/firmware/esp32_node/include/project_config_local.h`

and set your local WiFi and broker values there. The real override file is ignored by git.

### 2. Install the edge-listener dependency

```powershell
python -m pip install -r source/edge_server/mqtt_listener/requirements.txt
```

### 3. Build and flash the firmware

```powershell
pio run -d source/firmware/esp32_node
pio run -d source/firmware/esp32_node -t upload
```

### 4. Run the local MQTT listener

```powershell
python source/edge_server/mqtt_listener/listen_aggregates.py --host <BROKER_HOST> --port 1883 --topic project/adaptive-sampling-node/aggregate --csv source/results/summaries/latest_listener.csv --jsonl source/results/summaries/latest_listener.jsonl
```

## Professional-Repo Notes

- Live secrets were removed from the committed firmware config.
- Build output, Python cache files, local logs, and workspace-only tooling are ignored.
- The README states clearly what is already validated and what is still pending, so the repository does not overclaim.

## Remaining Final-Submission Work

- capture one real `LoRaWAN + TTN` uplink on campus
- complete the baseline-vs-adaptive energy comparison with a meter
- save one live `MQTTS` validation run
- curate the final prompt history in [`source/prompts/`](./source/prompts/)
