# Evidence Screenshot Checklist

This file lists the screenshots and visual evidence we should capture for the final submission and workshop demo.

The key rule is simple:

- every screenshot should prove one specific claim
- every screenshot should have a short caption
- every screenshot should have a clear filename

## Naming Convention

Recommended filename pattern:

`YYYY-MM-DD_<topic>_<what-it-proves>.png`

Examples:

- `2026-04-18_fft_adaptive_update_serial.png`
- `2026-04-18_mqtt_listener_clean_profile.png`
- `2026-04-20_ttn_uplink_received.png`

## Storage Convention

Recommended destination folders:

- final README figures: `pics/`
- raw workshop screenshots: `pics/`
- if the number grows large, create subfolders later such as:
  - `pics/runtime/`
  - `pics/mqtt/`
  - `pics/ttn/`
  - `pics/power/`

## Mandatory Screenshots

These are the screenshots most likely to help during evaluation.

### 1. Architecture / System Overview

What to capture:

- one diagram or clear figure showing:
  virtual signal -> sampling -> FFT -> adaptive control -> aggregate -> MQTT / LoRaWAN

Why it matters:

- helps the evaluator understand the whole pipeline in one glance

Suggested filename:

- `architecture_pipeline_overview.png`

Suggested caption:

- `Overall adaptive-sampling pipeline from signal generation to edge/cloud transmission.`

### 2. Maximum Sampling Frequency Evidence

What to capture:

- serial output or benchmark result showing the measured raw maximum sampling frequency

Why it matters:

- directly supports the grading item about maximum sampling frequency

Already captured:

- `../pics/Sampling_frequency.png`

Suggested filename:

- `max_sampling_frequency_result.png` or keep the existing `Sampling_frequency.png`

Suggested caption:

- `Raw maximum sampling benchmark on the Heltec board, measured separately from the stricter full-pipeline operating baseline.`

### 3. FFT Result And Adaptive Sampling Update

What to capture:

- serial output showing:
  - detected dominant frequency
  - adaptive sampling update from initial oversampling to the lower selected rate

Why it matters:

- proves the FFT-based adaptation is real, not just described

Suggested filename:

- `fft_adaptive_sampling_update.png`

Suggested caption:

- `FFT result and adaptive sampling update after detecting the dominant signal frequency.`

### 4. Aggregate Computation Evidence

What to capture:

- serial output showing:
  - window completion
  - average value
  - sample count
  - dominant frequency

Why it matters:

- supports the aggregate-function grading item

Suggested filename:

- `aggregate_window_result.png`

Suggested caption:

- `Per-window aggregate result including average value, sample count, and dominant frequency.`

### 5. MQTT Broker Connected

What to capture:

- serial output showing the board connected to WiFi and the MQTT broker

Why it matters:

- proves the edge path is truly live

Suggested filename:

- `mqtt_wifi_connected.png`

Suggested caption:

- `ESP32 connected to WiFi and MQTT broker on the home network.`

### 6. MQTT Message Received By Listener

What to capture:

- terminal output of the Python listener receiving a real aggregate

Why it matters:

- proves end-to-end edge transmission

Suggested filename:

- `mqtt_listener_received_message.png`

Suggested caption:

- `Edge listener receiving real aggregate messages from the ESP32 over MQTT.`

### 7. MQTT Summary Evidence

What to capture:

- the generated summary markdown or terminal output for one clean run

Why it matters:

- supports latency and communication evidence with saved artifacts

Suggested filename:

- `mqtt_summary_clean_profile.png`

Suggested caption:

- `Summary of MQTT latency and payload metrics for the clean profile run.`

### 8. Three Signal Profiles Evidence

What to capture:

- one screenshot per saved run or one combined table showing:
  - `clean_reference`
  - `noisy_reference`
  - `anomaly_stress`

Why it matters:

- directly supports the bonus section

Suggested filenames:

- `signal_profile_clean_summary.png`
- `signal_profile_noisy_summary.png`
- `signal_profile_anomaly_summary.png`

Suggested caption:

- `Saved measurement summary for the <profile> signal profile.`

### 9. Anomaly Injection Evidence

What to capture:

- serial log or MQTT listener output showing non-zero `anomaly_count`

Why it matters:

- proves the anomaly-stress profile is not just configured, but actually active

Suggested filename:

- `anomaly_profile_nonzero_count.png`

Suggested caption:

- `Anomaly-stress signal profile producing windows with non-zero anomaly counts.`

### 10. BetterSerialPlotter Live View

What to capture:

- BetterSerialPlotter showing at least:
  - `sampling_frequency_hz`
  - `dominant_frequency_hz`
  - `average_value`

Why it matters:

- gives a cleaner live visual explanation of the adaptive pipeline than raw serial text

Suggested filename:

- `better_serial_plotter_live_view.png`

Suggested caption:

- `BetterSerialPlotter visualization of the adaptive-sampling pipeline over time.`

### 11. Energy Measurement Setup Photo

What to capture:

- a photo of the real hardware setup used for power measurement

Already captured:

- `../pics/hardware.png`

Why it matters:

- makes the energy results more believable and easier to explain

Suggested filename:

- `power_measurement_hardware_setup.jpg`

Suggested caption:

- `Hardware setup used to measure power and energy consumption.`

### 12. Energy Graphs

What to capture:

- at least one graph for baseline / over-sampling
- at least one graph for adaptive sampling
- at least one graph for deep sleep or low-power behavior if used

Already captured:

- `../pics/2026-04-21_ina219_adaptive_betterserialplotter.png`
- `../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png`

Saved numeric evidence:

- `../results/summaries/ina219_comparison_2026-04-21.md`
- `../results/summaries/ina219_three_mode_comparison_2026-04-21.md`

Why it matters:

- directly supports the energy-savings grading item

Suggested filenames:

- `energy_baseline_run.png`
- `energy_adaptive_run.png`
- `energy_sleep_or_idle_run.png`

Suggested caption:

- `Power profile during <baseline/adaptive/deep-sleep> operation.`

### 13. Communication Volume Evidence

What to capture:

- Wireshark, tcpdump, broker statistics, or payload-size summary showing transmitted volume

Why it matters:

- directly supports the communication-cost grading item

Suggested filename:

- `network_volume_measurement.png`

Suggested caption:

- `Communication-volume evidence for MQTT aggregate transmission.`

### 14. Latency Measurement Evidence

What to capture:

- saved summary or console output showing listener latency and end-to-end latency

Why it matters:

- directly supports the latency grading item

Suggested filename:

- `latency_measurement_summary.png`

Suggested caption:

- `End-to-end latency summary computed from synchronized MQTT timestamps.`

### 15. TTN Uplink Received

What to capture near working gateway coverage:

- TTN console screenshot showing the real uplink
- decoded payload if possible

Already captured example files:

- `../pics/2026-04-20_ttn_live_data_uplink.png`
- `../pics/2026-04-20_ttn_uplink_decoded.png`
- `../pics/2026-04-20_ttn_device_overview.png`

Why it matters:

- this is the strongest proof for the cloud transmission requirement

Suggested filenames:

- `ttn_uplink_received.png`
- `ttn_decoded_payload.png`

Suggested caption:

- `LoRaWAN uplink received by TTN with decoded aggregate payload.`

## Nice-To-Have Screenshots

These are not strictly required, but they strengthen the story.

### 1. Board Boot And Mode Selection

What to capture:

- serial output showing:
  - firmware boot
  - communication mode
  - active signal profile

Suggested filename:

- `firmware_boot_mode_selection.png`

### 2. Queue / Supervisor Heartbeat

What to capture:

- one supervisor heartbeat showing system health

Suggested filename:

- `supervisor_heartbeat.png`

### 3. LoRaWAN Main-App Serial Proof

What to capture:

- serial output showing:
  - `joined=1`
  - `LoRaWAN uplink queued to radio`
  - `Event : Tx Done`
  - or `Prepared LoRaWAN aggregate payload`

Suggested filename:

- `lorawan_join_and_tx_done.png`

### 4. Runtime Notes Or Results Folder Snapshot

What to capture:

- a view of the saved result files in `results/summaries/`

Suggested filename:

- `saved_results_artifacts.png`

## Caption Writing Rule

Each caption should answer:

- what is shown
- what it proves

Good example:

- `MQTT listener receiving real aggregate messages from the Heltec board, confirming end-to-end edge transmission over WiFi.`

Weak example:

- `MQTT screenshot`

## Final Evidence Matrix

Before submission, we should be able to point to one screenshot or saved artifact for each of these:

- maximum stable sampling frequency
- FFT-based dominant frequency detection
- adaptive sampling update
- aggregate correctness
- MQTT end-to-end delivery
- LoRaWAN / TTN delivery
- energy measurement
- communication volume
- end-to-end latency
- all three signal profiles

If any one of those rows is missing evidence, it should be treated as unfinished.
