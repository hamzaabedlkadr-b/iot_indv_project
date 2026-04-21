# Final Evidence Index - 2026-04-21

This file is the short map for the evidence we should use in the final report and GitHub README.

## Good Maximum Frequency

Use this result for the maximum-frequency rubric item:

```text
Raw benchmark result | generated=199126 min_dt=3.0us max_dt=53.0us achieved=199126.59Hz stable=yes duration=1.00s
```

Evidence:

- [`../pics/Sampling_frequency.png`](../pics/Sampling_frequency.png)

Important wording:

- raw class-style maximum sampling benchmark: `199,126.59 Hz`
- strict full-pipeline clean baseline: `50 Hz`
- adaptive operating rate for the current signal: `40 Hz`

## Three-Signal Bonus

Use this table for the additional points about three input signals:

| Profile | Avg dominant frequency | Avg sampling frequency | Avg anomaly count | Avg payload size |
| --- | --- | --- | --- | --- |
| `clean_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `446.0 B` |
| `noisy_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `445.6 B` |
| `anomaly_stress` | `5.000 Hz` | `40.000 Hz` | `2.800` | `444.6 B` |

Evidence:

- [`summaries/signal_profile_comparison_2026-04-18.txt`](./summaries/signal_profile_comparison_2026-04-18.txt)
- [`screenshots/signal_profile_comparison_2026-04-18.png`](./screenshots/signal_profile_comparison_2026-04-18.png)
- [`summaries/mqtt_summary_2026-04-17_clean_profile_synced.md`](./summaries/mqtt_summary_2026-04-17_clean_profile_synced.md)
- [`summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md`](./summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md)
- [`summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md`](./summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md)
- [`screenshots/anomaly_profile_nonzero_count_2026-04-17.png`](./screenshots/anomaly_profile_nonzero_count_2026-04-17.png)

## Anomaly Filter Bonus

Use this artifact for the extended anomaly-aware filtering discussion:

- [`summaries/anomaly_filter_evaluation_2026-04-21.md`](./summaries/anomaly_filter_evaluation_2026-04-21.md)
- [`summaries/anomaly_filter_metrics_2026-04-21.csv`](./summaries/anomaly_filter_metrics_2026-04-21.csv)
- [`summaries/anomaly_filter_window_tradeoff_2026-04-21.csv`](./summaries/anomaly_filter_window_tradeoff_2026-04-21.csv)

Key coverage:

- anomaly probabilities: `p = 1%, 5%, 10%`
- filters: `Z-score` and `Hampel`
- metrics: `TPR`, `FPR`, mean-error reduction, FFT dominant-frequency error, adaptive-rate impact, execution time, estimated filter energy
- window-size tradeoff: Hampel windows `5`, `11`, `21`, and `41`

## Fresh WiFi/MQTT Run

Use this run to prove the current WiFi setup works after changing network:

- [`wifi_mqtt_evidence_2026-04-21.md`](./wifi_mqtt_evidence_2026-04-21.md)
- [`summaries/mqtt_run_2026-04-21_fresh_wifi.csv`](./summaries/mqtt_run_2026-04-21_fresh_wifi.csv)
- [`summaries/mqtt_run_2026-04-21_fresh_wifi.jsonl`](./summaries/mqtt_run_2026-04-21_fresh_wifi.jsonl)
- [`summaries/mqtt_summary_2026-04-21_fresh_wifi.md`](./summaries/mqtt_summary_2026-04-21_fresh_wifi.md)

Key values:

- messages received: `6`
- missing windows: `0`
- average sampling frequency: `40.000 Hz`
- average dominant frequency: `5.000 Hz`
- average payload size: `401.667 B`

## Communication Volume

Use this table for the network-volume rubric item:

- [`summaries/communication_volume_comparison_2026-04-21.md`](./summaries/communication_volume_comparison_2026-04-21.md)

Key values for the five-window comparison:

| Mode | Sampling rate | Samples represented | MQTT messages | Total payload |
| --- | --- | --- | --- | --- |
| Fixed baseline | `50 Hz` | `1250` | `5` | `2270 B` |
| Adaptive | `40 Hz` | `1000` | `5` | `2270 B` |

Important wording:

- adaptive sampling reduces the represented local samples by `20%`
- MQTT bytes stay effectively constant because the system transmits one aggregate per completed window
- this is expected and good: aggregation prevents network traffic from scaling with raw sample count

## LoRaWAN / TTN Proof

Use these for cloud delivery:

- [`lorawan_evidence_2026-04-20.md`](./lorawan_evidence_2026-04-20.md)
- [`../pics/2026-04-20_serial_lorawan_join_tx.png`](../pics/2026-04-20_serial_lorawan_join_tx.png)
- [`../pics/2026-04-20_serial_lorawan_payload.png`](../pics/2026-04-20_serial_lorawan_payload.png)
- [`../pics/2026-04-20_ttn_live_data_uplink.png`](../pics/2026-04-20_ttn_live_data_uplink.png)
- [`../pics/2026-04-20_ttn_uplink_decoded.png`](../pics/2026-04-20_ttn_uplink_decoded.png)
- [`../pics/2026-04-20_ttn_device_overview.png`](../pics/2026-04-20_ttn_device_overview.png)

## Architecture And Visuals

Use these screenshots for the README and workshop story:

- [`../pics/architecture_pipeline_overview.png`](../pics/architecture_pipeline_overview.png)
- [`../pics/2026-04-18_better_serial_plotter_live_view.png`](../pics/2026-04-18_better_serial_plotter_live_view.png)
- [`../pics/Sampling_frequency.png`](../pics/Sampling_frequency.png)

## Energy / INA219 Proof

Use these files for the energy-saving rubric item:

- [`summaries/ina219_baseline_2026-04-21.tsv`](./summaries/ina219_baseline_2026-04-21.tsv)
- [`summaries/ina219_baseline_2026-04-21.md`](./summaries/ina219_baseline_2026-04-21.md)
- [`summaries/ina219_adaptive_2026-04-21.tsv`](./summaries/ina219_adaptive_2026-04-21.tsv)
- [`summaries/ina219_adaptive_2026-04-21.md`](./summaries/ina219_adaptive_2026-04-21.md)
- [`summaries/ina219_comparison_2026-04-21.md`](./summaries/ina219_comparison_2026-04-21.md)
- [`summaries/ina219_deepsleep_2026-04-21.md`](./summaries/ina219_deepsleep_2026-04-21.md)
- [`summaries/ina219_three_mode_comparison_2026-04-21.md`](./summaries/ina219_three_mode_comparison_2026-04-21.md)
- [`../pics/hardware.png`](../pics/hardware.png)
- [`../pics/2026-04-21_ina219_adaptive_betterserialplotter.png`](../pics/2026-04-21_ina219_adaptive_betterserialplotter.png)
- [`../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png`](../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png)

Key values:

- baseline fixed `50 Hz`: `553.0000 mW`, `18.433238 mWh` over `119.995 s`
- adaptive `40 Hz`: `552.6775 mW`, `18.422466 mWh` over `119.995 s`
- energy delta: `-0.06%`
- peak power delta: `-9.16%`
- optional adaptive + deep sleep: `410.8682 mW`, `13.632451 mWh` over `119.480 s`
- optional deep-sleep energy delta: `-26.04%`

Important wording:

- adaptive sampling reduces the sampling/processing rate, but WiFi, display, and always-on FreeRTOS tasks dominate the measured power profile
- the small average energy change is still a valid result because both runs were measured under the same hardware and workload conditions
- deep sleep is an additional low-power strategy and should be presented separately from the required adaptive-sampling comparison

## Still Pending

- live secure MQTT proof if a TLS broker is available

Power-test reference plan:

- [`reference_repo_power_conditions_2026-04-21.md`](./reference_repo_power_conditions_2026-04-21.md)
- [`../docs/ENERGY_MEASUREMENT_RUNBOOK.md`](../docs/ENERGY_MEASUREMENT_RUNBOOK.md)
