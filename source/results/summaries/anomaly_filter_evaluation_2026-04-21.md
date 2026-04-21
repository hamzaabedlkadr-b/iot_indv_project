# Anomaly Filter Evaluation

This deterministic host-side evaluation mirrors the firmware signal model:

- clean signal: `2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)`
- noise: near-Gaussian baseline noise with `sigma = 0.2`
- anomaly process: sparse spikes with `p = 1%, 5%, 10%` and magnitude `+/-U(5, 15)`
- window: `5 s` at the adaptive operating rate, so `200` samples per window

The energy column is an active-power estimate using the measured adaptive INA219 average power. It is not a separate physical INA219 run for each filter, but it gives a consistent per-window cost estimate for the extra CPU work.

## Detection And Signal Quality

| Filter | p | Window | TPR | FPR | Mean error reduction | FFT error before | FFT error after | Adaptive rate after | Exec time | Est. energy |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| zscore | 1% | 200 | 72.28% | 0.00% | 49.06% | `0.000 Hz` | `0.000 Hz` | `40.0 Hz` | `668.7 us` | `0.1027 uWh` |
| hampel | 1% | 11 | 61.88% | 0.42% | 5.42% | `0.000 Hz` | `0.000 Hz` | `40.0 Hz` | `1626.6 us` | `0.2497 uWh` |
| zscore | 5% | 200 | 67.25% | 0.00% | 35.98% | `0.000 Hz` | `0.000 Hz` | `40.0 Hz` | `1848.3 us` | `0.2838 uWh` |
| hampel | 5% | 11 | 60.69% | 1.06% | 0.71% | `0.000 Hz` | `0.000 Hz` | `40.0 Hz` | `2162.5 us` | `0.3320 uWh` |
| zscore | 10% | 200 | 58.27% | 0.00% | 34.61% | `0.000 Hz` | `0.000 Hz` | `40.0 Hz` | `1759.1 us` | `0.2701 uWh` |
| hampel | 10% | 11 | 57.03% | 1.70% | 9.43% | `0.000 Hz` | `0.000 Hz` | `40.0 Hz` | `2201.2 us` | `0.3379 uWh` |

## Hampel Window Trade-Off

| p | Hampel window | TPR | FPR | Delay added | State memory | Exec time | Mean error reduction |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1% | 5 | 35.15% | 0.08% | `0.050 s` | `20 B` | `1460.5 us` | 14.39% |
| 1% | 11 | 61.88% | 0.42% | `0.125 s` | `44 B` | `2133.6 us` | 5.42% |
| 1% | 21 | 56.93% | 0.01% | `0.250 s` | `84 B` | `3533.4 us` | 33.57% |
| 1% | 41 | 60.40% | 0.00% | `0.500 s` | `164 B` | `5916.5 us` | 39.10% |
| 5% | 5 | 40.39% | 0.15% | `0.050 s` | `20 B` | `1494.2 us` | 19.73% |
| 5% | 11 | 60.69% | 1.06% | `0.125 s` | `44 B` | `2136.2 us` | 0.71% |
| 5% | 21 | 60.98% | 0.07% | `0.250 s` | `84 B` | `3449.7 us` | 30.00% |
| 5% | 41 | 62.25% | 0.01% | `0.500 s` | `164 B` | `6452.2 us` | 30.79% |
| 10% | 5 | 39.50% | 0.26% | `0.050 s` | `20 B` | `1531.7 us` | 14.29% |
| 10% | 11 | 57.03% | 1.70% | `0.125 s` | `44 B` | `2098.6 us` | 9.43% |
| 10% | 21 | 59.36% | 0.07% | `0.250 s` | `84 B` | `3483.0 us` | 35.39% |
| 10% | 41 | 60.59% | 0.00% | `0.500 s` | `164 B` | `5659.2 us` | 32.77% |

## Interpretation

- `Z-score` is the cheapest option and performs well for this low-frequency sinusoidal signal, but its detection rate drops as spike contamination rises because the window mean and standard deviation are affected by outliers.
- `Hampel` gives a tunable robustness/delay trade-off: larger windows reduce false positives and improve mean-error reduction, but they add centered-filter delay and require more state.
- The dominant FFT component remains close to `5 Hz` after filtering, so the adaptive policy remains stable at `40 Hz` instead of being pulled toward a higher noisy bin.
- Larger Hampel windows improve statistical context but increase centered-filter delay by roughly `window/2` samples and increase memory linearly.
