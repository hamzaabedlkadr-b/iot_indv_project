# Grading Evidence Matrix

This file maps the grading rubric to the artifacts that already exist in the repository.

Status legend:

- `Validated`: supported by a real saved run or measurement artifact
- `Partial`: some evidence exists, but the final comparison or live proof is incomplete
- `Pending`: still needs a live run or a missing implementation detail

| Rubric item | Points | Current status | Best current evidence | What still needs to happen |
| --- | --- | --- | --- | --- |
| Compute correctly the max frequency of the input signal | `15` | Validated | [`PHASE1_REPORT.tex`](./PHASE1_REPORT.tex) documents the strict stable point at `50 Hz` and instability above it | Capture or keep one screenshot of the benchmark table / serial result for the workshop |
| Correctly compute optimal frequency | `15` | Validated | [`PHASE1_REPORT.tex`](./PHASE1_REPORT.tex) and [`../pics/2026-04-18_better_serial_plotter_live_view.png`](../pics/2026-04-18_better_serial_plotter_live_view.png) show the `5 Hz -> 40 Hz` adaptive result | Keep one clean screenshot of the adaptive update line or plotter view |
| Compute correctly the average function over window | `10` | Validated | [`../results/runtime_notes_2026-04-17.md`](../results/runtime_notes_2026-04-17.md), MQTT payload summaries, and aggregate logs | Keep one screenshot showing `average_value`, `sample_count`, and `window_id` together |
| Evaluate correctly the saving in energy | `10` | Pending | [`ENERGY_MEASUREMENT_RUNBOOK.md`](./ENERGY_MEASUREMENT_RUNBOOK.md) and [`MEASUREMENT_PLAN.md`](./MEASUREMENT_PLAN.md) define the method | Run the fixed baseline and adaptive measurements with a USB power meter or equivalent |
| Measure correctly the volume of data transmitted over the network | `5` | Partial | [`../results/summaries/mqtt_summary_2026-04-18_listener.md`](../results/summaries/mqtt_summary_2026-04-18_listener.md) gives current payload size; profile summaries give per-run bytes | Complete the baseline-vs-adaptive comparison table and explain that one aggregate per window keeps payload volume nearly constant |
| Measure correctly the end-to-end latency | `5` | Validated | [`../results/summaries/mqtt_summary_2026-04-18_listener.md`](../results/summaries/mqtt_summary_2026-04-18_listener.md) and [`../results/mqtt_evidence_2026-04-18.md`](../results/mqtt_evidence_2026-04-18.md) | Take or reuse one screenshot of the saved summary |
| Transmit the result to the MQTT server in a secure way | `10` | Partial | [`SECURE_MQTT_SETUP.md`](./SECURE_MQTT_SETUP.md) and the updated MQTT client support `mqtts://`, certificate verification, and optional credentials | Validate the TLS path against a real secure broker and save one proof run |
| Quality of the FreeRTOS code | `5` | Validated | Modular task pipeline under [`../firmware/esp32_node/`](../firmware/esp32_node/), queue separation, comments, and component READMEs | Be ready to explain task ownership, queues, and timing decisions in the workshop |
| Quality of presentation of project in the GitHub repository | `5` | Partial | [`../README.md`](../README.md), [`PRESENTATION_PLAYBOOK.md`](./PRESENTATION_PLAYBOOK.md), [`EVIDENCE_SCREENSHOT_CHECKLIST.md`](./EVIDENCE_SCREENSHOT_CHECKLIST.md) | Add the remaining screenshots and fill the final README figures |
| Consider three different input signals | `20` | Validated | [`../results/summaries/mqtt_summary_2026-04-17_clean_profile_synced.md`](../results/summaries/mqtt_summary_2026-04-17_clean_profile_synced.md), [`../results/summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md`](../results/summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md), [`../results/summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md`](../results/summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md) | Capture one screenshot per profile or a combined comparison table |

## Most Important Missing Proofs

If time is short, the highest-value missing items are:

1. `Energy` comparison screenshots and numbers
2. `TTN` campus uplink screenshot
3. `Secure MQTT` validation if a TLS-capable broker is available

## Recommended Workshop Order

1. Show the architecture and signal profiles from the main README.
2. Show the `50 Hz` maximum stable benchmark result.
3. Show the adaptive update to `40 Hz`.
4. Show the aggregate and MQTT listener evidence.
5. Show the three signal-profile summaries.
6. Show energy and TTN once those final artifacts are collected.
