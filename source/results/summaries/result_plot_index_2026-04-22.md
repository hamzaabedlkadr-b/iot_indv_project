# Final Result Plots - 2026-04-22

These plots are generated from the saved measurement summaries using:

```powershell
python source\results\plot_final_results.py
```

| Plot | Source data | Purpose |
| --- | --- | --- |
| [`../../pics/result_energy_comparison_2026-04-22.png`](../../pics/result_energy_comparison_2026-04-22.png) | `ina219_*_2026-04-21.json` | Energy and power comparison across baseline, adaptive, and deep sleep |
| [`../../pics/result_communication_volume_2026-04-22.png`](../../pics/result_communication_volume_2026-04-22.png) | `communication_volume_comparison_2026-04-21.md` values | Fixed-baseline vs adaptive sample and MQTT payload volume |
| [`../../pics/result_mqtt_latency_2026-04-22.png`](../../pics/result_mqtt_latency_2026-04-22.png) | `mqtt_summary_2026-04-18_listener.json` | Average and p95 MQTT latency |
| [`../../pics/result_anomaly_filters_2026-04-22.png`](../../pics/result_anomaly_filters_2026-04-22.png) | `anomaly_filter_metrics_2026-04-21.csv` | Z-score vs Hampel detection and error-reduction results |
