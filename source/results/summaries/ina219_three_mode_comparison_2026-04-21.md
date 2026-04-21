# INA219 Three-Mode Energy Comparison - 2026-04-21

This is an extra low-power experiment. It should be presented separately from the required fixed-vs-adaptive comparison because deep sleep changes the duty cycle of the whole board, not only the sampling frequency.

## Runs Compared

| Run | Firmware condition | Duration | Evidence |
| --- | --- | --- | --- |
| Baseline | Fixed `50 Hz`, awake | `119.995 s` | [`ina219_baseline_2026-04-21.md`](./ina219_baseline_2026-04-21.md) |
| Adaptive | Adaptive `40 Hz`, awake | `119.995 s` | [`ina219_adaptive_2026-04-21.md`](./ina219_adaptive_2026-04-21.md) |
| Adaptive + deep sleep | Adaptive pipeline, then timed deep sleep cycles | `119.480 s` | [`ina219_deepsleep_2026-04-21.md`](./ina219_deepsleep_2026-04-21.md) |

## Results

| Metric | Baseline `50 Hz` | Adaptive `40 Hz` | Adaptive + deep sleep |
| --- | ---: | ---: | ---: |
| Average current | `110.4953 mA` | `110.6984 mA` | `81.7522 mA` |
| Average power | `553.0000 mW` | `552.6775 mW` | `410.8682 mW` |
| Integrated energy | `18.433238 mWh` | `18.422466 mWh` | `13.632451 mWh` |
| Peak power | `873.0000 mW` | `793.0000 mW` | `675.0000 mW` |
| Minimum power | `503.0000 mW` | `505.0000 mW` | `48.0000 mW` |

## Delta Versus Baseline

| Metric | Adaptive `40 Hz` | Adaptive + deep sleep |
| --- | ---: | ---: |
| Average current | `+0.18%` | `-26.01%` |
| Average power | `-0.06%` | `-25.70%` |
| Integrated energy | `-0.06%` | `-26.04%` |
| Peak power | `-9.16%` | `-22.68%` |
| Minimum power | `+0.40%` | `-90.46%` |

## Interpretation

The direct adaptive-sampling comparison shows only a small average energy change because WiFi, display, MQTT, and FreeRTOS runtime overhead dominate the Heltec board power draw while the board remains awake.

The deep-sleep experiment is much stronger electrically: the timed sleep cycles reduce average energy by about `26%` over the same measurement length and create clear low-power valleys down to `48 mW`. This is a useful optional optimization discussion, but it should not replace the required baseline-versus-adaptive sampling comparison.
