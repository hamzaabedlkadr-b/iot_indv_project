# Results

This folder stores the artifacts used in the final report.

Available offline tooling:

- [`analyze_mqtt_results.py`](./analyze_mqtt_results.py) summarizes the `CSV` logs produced by the edge listener
- [`compare_communication_volume.py`](./compare_communication_volume.py) generates the fixed-baseline versus adaptive network-volume comparison
- [`anomaly_filter_evaluation.py`](./anomaly_filter_evaluation.py) generates the `Z-score` and `Hampel` anomaly-filter bonus metrics
- [`plot_input_signal_profiles.py`](./plot_input_signal_profiles.py) regenerates the input-signal plots and sample `CSV`
- [`plot_final_results.py`](./plot_final_results.py) regenerates the final result plots from saved measurement summaries
- [`convert_mosquitto_log_to_csv.py`](./convert_mosquitto_log_to_csv.py) converts timestamped `mosquitto_sub` captures into the same `CSV` format
- [`measurement_summary_template.md`](./measurement_summary_template.md) provides a report-ready structure for the required metrics

The current `CSV` format also preserves `signal_profile` and `anomaly_count`, which makes it easier to compare the three bonus signal runs without manually renaming files later.

Current visual evidence bundles:

- [`lorawan_evidence_2026-04-20.md`](./lorawan_evidence_2026-04-20.md)
- [`mqtt_evidence_2026-04-18.md`](./mqtt_evidence_2026-04-18.md)
- [`secure_mqtt_evidence_2026-04-22.md`](./secure_mqtt_evidence_2026-04-22.md)
- [`../docs/EVIDENCE_SCREENSHOT_CHECKLIST.md`](../docs/EVIDENCE_SCREENSHOT_CHECKLIST.md)
- [`screenshot_inventory_2026-04-18.md`](./screenshot_inventory_2026-04-18.md)

Typical workflow after a live `MQTT` run:

1. Prefer the Python listener so the full JSON payload and receive timestamp are captured together.
2. Run:
   `python source\results\analyze_mqtt_results.py --input source\edge_server\mqtt_listener\logs\aggregates.csv --output-json source\results\summaries\mqtt_summary.json --output-md source\results\summaries\mqtt_summary.md`
3. Copy the resulting numbers into the measurement summary template.

If you captured the run with `mosquitto_sub` instead of the Python listener:

1. Convert the raw log:
   `python source\results\convert_mosquitto_log_to_csv.py --input source\edge_server\mqtt_listener\logs\mqtt_run_raw.log --output source\edge_server\mqtt_listener\logs\mqtt_run.csv`
2. Analyze the generated CSV with `analyze_mqtt_results.py`.

Reference artifacts from the first clean home-network timing run:

- `summaries/mqtt_run_2026-04-17_listener_clean.csv`
- `summaries/mqtt_run_2026-04-17_listener_clean.jsonl`
- `summaries/mqtt_summary_2026-04-17_listener_clean.json`
- `summaries/mqtt_summary_2026-04-17_listener_clean.md`
- `summaries/mqtt_run_2026-04-17_clean_profile_synced.csv`
- `summaries/mqtt_run_2026-04-17_clean_profile_synced.jsonl`
- `summaries/mqtt_summary_2026-04-17_clean_profile_synced.json`
- `summaries/mqtt_summary_2026-04-17_clean_profile_synced.md`
- `summaries/mqtt_run_2026-04-17_noisy_profile_synced.csv`
- `summaries/mqtt_run_2026-04-17_noisy_profile_synced.jsonl`
- `summaries/mqtt_summary_2026-04-17_noisy_profile_synced.json`
- `summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md`
- `summaries/mqtt_run_2026-04-17_anomaly_profile_synced.csv`
- `summaries/mqtt_run_2026-04-17_anomaly_profile_synced.jsonl`
- `summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.json`
- `summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md`
- `better_serial_plotter_sample_2026-04-18.txt`

Reference artifacts from the follow-up live MQTT evidence run on `2026-04-18`:

- `summaries/mqtt_run_2026-04-18_listener.csv`
- `summaries/mqtt_run_2026-04-18_listener.jsonl`
- `summaries/mqtt_summary_2026-04-18_listener.json`
- `summaries/mqtt_summary_2026-04-18_listener.md`
- `summaries/mqtt_listener_2026-04-18_live.txt`
- `mqtt_evidence_2026-04-18.md`
- `summaries/communication_volume_comparison_2026-04-21.md`

Reference artifacts from the secure MQTT validation run on `2026-04-22`:

- `secure_mqtt_evidence_2026-04-22.md`
- `summaries/secure_mqtt_listener_2026-04-22.out.txt`
- `summaries/secure_mqtt_run_final_2026-04-22.csv`
- `summaries/secure_mqtt_summary_final_2026-04-22.md`
- `summaries/secure_mqtt_summary_final_2026-04-22.json`
- `../pics/2026-04-22_secure_mqtt_listener_tls.png`
- `../pics/2026-04-22_secure_mqtt_cert_validated.png`
- `../pics/2026-04-22_secure_mqtt_heltec_sent.png`

Reference artifacts for the input-signal and anomaly-filter bonus:

- `summaries/input_signal_samples_2026-04-22.csv`
- `../pics/input_signal_clean_reference_2026-04-22.png`
- `../pics/input_signal_profiles_2026-04-22.png`
- `summaries/anomaly_filter_evaluation_2026-04-21.md`
- `summaries/anomaly_filter_evaluation_2026-04-21.json`
- `summaries/anomaly_filter_metrics_2026-04-21.csv`
- `summaries/anomaly_filter_window_tradeoff_2026-04-21.csv`

Reference artifacts for final result plots:

- `summaries/result_plot_index_2026-04-22.md`
- `../pics/result_energy_comparison_2026-04-22.png`
- `../pics/result_communication_volume_2026-04-22.png`
- `../pics/result_mqtt_latency_2026-04-22.png`
- `../pics/result_anomaly_filters_2026-04-22.png`

Reference artifacts from the integrated `LoRaWAN + TTN` validation run on `2026-04-20`:

- `lorawan_evidence_2026-04-20.md`
- `../pics/2026-04-20_serial_lorawan_join_tx.png`
- `../pics/2026-04-20_serial_lorawan_payload.png`
- `../pics/2026-04-20_ttn_live_data_uplink.png`
- `../pics/2026-04-20_ttn_uplink_decoded.png`
- `../pics/2026-04-20_ttn_device_overview.png`

Examples of artifacts to keep here:

- `CSV` measurements
- summary `JSON` or `Markdown`
- screenshots
- latency summaries
- energy comparison tables
- bonus experiment outputs
