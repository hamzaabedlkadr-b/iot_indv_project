# Grading Evidence Matrix

This file maps the grading rubric to the artifacts that already exist in the repository.

Status legend:

- `Validated`: supported by a real saved run or measurement artifact
- `Partial`: some evidence exists, but the final comparison or live proof is incomplete
- `Pending`: still needs a live run or a missing implementation detail

| Rubric item | Points | Current status | Best current evidence | What still needs to happen |
| --- | --- | --- | --- | --- |
| Compute correctly the max frequency of the input signal | `15` | Validated | [`../pics/Sampling_frequency.png`](../pics/Sampling_frequency.png) shows the raw benchmark at `199,126.59 Hz`; [`CURRENT_PROGRESS_REPORT.md`](./CURRENT_PROGRESS_REPORT.md) explains why `50 Hz` is still used as the strict full-pipeline baseline | Keep the raw benchmark screenshot and be ready to explain the difference between raw throughput and full-pipeline stability |
| Correctly compute optimal frequency | `15` | Validated | [`PHASE1_REPORT.tex`](./PHASE1_REPORT.tex), [`../pics/2026-04-18_better_serial_plotter_live_view.png`](../pics/2026-04-18_better_serial_plotter_live_view.png), and [`../results/wifi_mqtt_evidence_2026-04-21.md`](../results/wifi_mqtt_evidence_2026-04-21.md) show the `5 Hz -> 40 Hz` adaptive result | Keep one clean screenshot of the adaptive update line or plotter view |
| Compute correctly the average function over window | `10` | Validated | [`../results/runtime_notes_2026-04-17.md`](../results/runtime_notes_2026-04-17.md), MQTT payload summaries, and aggregate logs | Keep one screenshot showing `average_value`, `sample_count`, and `window_id` together |
| Evaluate correctly the saving in energy | `10` | Validated | [`../pics/result_energy_comparison_2026-04-22.png`](../pics/result_energy_comparison_2026-04-22.png), [`../results/summaries/ina219_comparison_2026-04-21.md`](../results/summaries/ina219_comparison_2026-04-21.md), [`../results/summaries/ina219_three_mode_comparison_2026-04-21.md`](../results/summaries/ina219_three_mode_comparison_2026-04-21.md), [`../pics/hardware.png`](../pics/hardware.png), and the `INA219` plotter screenshots | Explain that WiFi/display/FreeRTOS activity dominated the awake adaptive result (`-0.06%`), while optional deep sleep reduced energy by `-26.04%` |
| Measure correctly the volume of data transmitted over the network | `5` | Validated | [`../pics/result_communication_volume_2026-04-22.png`](../pics/result_communication_volume_2026-04-22.png), [`../results/summaries/communication_volume_comparison_2026-04-21.md`](../results/summaries/communication_volume_comparison_2026-04-21.md) compares fixed `50 Hz` against adaptive `40 Hz`; [`../results/summaries/mqtt_summary_2026-04-18_listener.md`](../results/summaries/mqtt_summary_2026-04-18_listener.md) gives the measured adaptive payload bytes | Explain that represented samples drop by `20%`, while MQTT bytes stay flat because the design sends one aggregate per window |
| Measure correctly the end-to-end latency | `5` | Validated | [`../pics/result_mqtt_latency_2026-04-22.png`](../pics/result_mqtt_latency_2026-04-22.png), [`../results/summaries/mqtt_summary_2026-04-18_listener.md`](../results/summaries/mqtt_summary_2026-04-18_listener.md), and [`../results/mqtt_evidence_2026-04-18.md`](../results/mqtt_evidence_2026-04-18.md) | Use the latency result plot plus the saved summary |
| Transmit the result to the MQTT server in a secure way | `10` | Validated | [`../results/secure_mqtt_evidence_2026-04-22.md`](../results/secure_mqtt_evidence_2026-04-22.md), [`SECURE_MQTT_SETUP.md`](./SECURE_MQTT_SETUP.md), [`../pics/2026-04-22_secure_mqtt_listener_tls.png`](../pics/2026-04-22_secure_mqtt_listener_tls.png), and [`../pics/2026-04-22_secure_mqtt_cert_validated.png`](../pics/2026-04-22_secure_mqtt_cert_validated.png) show `MQTTS`, `tls=enabled verify=required`, and ESP32 certificate validation | Present this after the plain MQTT edge evidence so the secure transport proof is separate and easy to grade |
| Quality of the FreeRTOS code | `5` | Validated | Modular task pipeline under [`../firmware/esp32_node/`](../firmware/esp32_node/), queue separation, comments, and component READMEs | Be ready to explain task ownership, queues, and timing decisions in the workshop |
| Quality of presentation of project in the GitHub repository | `5` | Validated | [`../../README.md`](../../README.md), [`../README.md`](../README.md), [`../results/final_evidence_index_2026-04-21.md`](../results/final_evidence_index_2026-04-21.md), [`../results/lorawan_evidence_2026-04-20.md`](../results/lorawan_evidence_2026-04-20.md), [`../results/wifi_mqtt_evidence_2026-04-21.md`](../results/wifi_mqtt_evidence_2026-04-21.md), and the `INA219` evidence bundle | Keep the README tables and screenshot links synchronized if secure-MQTT evidence is added later |
| Consider three different input signals | `20` | Validated | [`../pics/input_signal_profiles_2026-04-22.png`](../pics/input_signal_profiles_2026-04-22.png), [`../pics/result_anomaly_filters_2026-04-22.png`](../pics/result_anomaly_filters_2026-04-22.png), [`../results/final_evidence_index_2026-04-21.md`](../results/final_evidence_index_2026-04-21.md), [`../results/summaries/signal_profile_comparison_2026-04-18.txt`](../results/summaries/signal_profile_comparison_2026-04-18.txt), [`../results/summaries/anomaly_filter_evaluation_2026-04-21.md`](../results/summaries/anomaly_filter_evaluation_2026-04-21.md), [`../results/screenshots/signal_profile_comparison_2026-04-18.png`](../results/screenshots/signal_profile_comparison_2026-04-18.png), and the three synced profile summaries | Show the waveform plot, the three-profile MQTT table, and the Z-score/Hampel anomaly-filter chart |

## Most Important Final Proofs

The highest-value proof screenshots to show in class are:

1. raw maximum sampling at `199,126.59 Hz`
2. adaptive `5 Hz -> 40 Hz` FFT result
3. secure MQTT listener and certificate-validation screenshots
4. LoRaWAN/TTN live uplink screenshots
5. INA219 energy comparison and deep-sleep comparison

## Recommended Workshop Order

1. Show the architecture and signal profiles from the main README.
2. Show the raw `199,126.59 Hz` maximum sampling benchmark, then explain that `50 Hz` is the conservative full-pipeline baseline.
3. Show the adaptive update to `40 Hz`.
4. Show the aggregate and MQTT listener evidence.
5. Show the LoRa serial proof and `TTN` screenshots from `2026-04-20`.
6. Show the three signal-profile summaries and the anomaly-filter evaluation.
7. Show the communication-volume comparison.
8. Close with the INA219 energy comparison and the secure-MQTT validation screenshot.
