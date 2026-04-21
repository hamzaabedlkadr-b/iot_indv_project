# Communication Volume Comparison

Source adaptive MQTT summary: `source\results\summaries\mqtt_summary_2026-04-18_listener.json`

The firmware transmits one aggregate message per completed window. The adaptive sampler changes how many local samples are generated and processed inside the window, but it intentionally does not transmit raw samples.

| Mode | Sampling rate | Window count | Samples represented | MQTT messages | Avg payload | Total payload | Delta vs fixed |
|---|---:|---:|---:|---:|---:|---:|---:|
| Fixed oversampling baseline | `50.0 Hz` | `5` | `1250` | `5` | `454.0 B` | `2270 B` | reference |
| Adaptive sampling | `40.0 Hz` | `5` | `1000` | `5` | `454.0 B` | `2270 B` | `0.00%` |

## Interpretation

- Local sample generation drops from `1250` to `1000` samples for the same `5` aggregate windows (`-20.00%`).
- MQTT network volume stays effectively constant because both modes publish one compact aggregate per window instead of raw samples.
- This is the intended architecture: adaptive sampling saves local sensing/processing work, while aggregation prevents network traffic from scaling with the raw sampling rate.
- If the system transmitted every raw sample instead of aggregates, adaptive sampling would reduce raw sample traffic by the same `20%` as the local sample count.
