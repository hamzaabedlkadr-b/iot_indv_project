# INA219 Baseline vs Adaptive Comparison

| Metric | Baseline | Adaptive | Delta vs baseline |
| --- | --- | --- | --- |
| Duration (s) | `119.995000` | `119.995000` | `0.00%` |
| Average current (mA) | `110.495283` | `110.698370` | `0.18%` |
| Average power (mW) | `553.000000` | `552.677530` | `-0.06%` |
| Integrated energy (mWh) | `18.433238` | `18.422466` | `-0.06%` |
| Peak current (mA) | `174.100000` | `159.900000` | `-8.16%` |
| Peak power (mW) | `873.000000` | `793.000000` | `-9.16%` |

Interpretation:
- Negative deltas for average current, average power, or integrated energy mean the adaptive run consumed less than the fixed baseline.
- Network-volume comparison should still be taken from the MQTT listener summaries because the INA219 measures electrical behavior, not payload bytes.
