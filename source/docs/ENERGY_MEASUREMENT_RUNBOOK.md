# Energy Measurement Runbook

This file turns the remaining energy requirement into a short, repeatable procedure.

If you use the `INA219` current / power monitor mentioned on the course page, also read:

- `docs/INA219_ENERGY_SETUP.md`

## Goal

Compare:

- fixed oversampling baseline at `50 Hz`
- adaptive sampling mode that drops to `40 Hz` for the current clean signal

For this repository, that is the correct graded energy test.

This matches the strongest pattern from the instructor-shared reference repositories: run the real signal-processing and communication workload on a device under test, measure current / power with an external monitor such as `INA219`, and compare fixed oversampling against the adaptive version under the same conditions.

Do this with:

- the same DUT board
- the same `clean_reference` signal profile
- the same `MQTT` communication path
- the same run duration

Do **not** replace this with a deep-sleep-only comparison. Deep sleep can be discussed as an extra low-power technique, but the required comparison here is the main project pipeline in fixed baseline mode versus adaptive mode.

The comparison should be done with the same board, same signal profile, same communication path, and the same run duration.

## Recommended Setup

- `Board A`: Heltec / ESP32 DUT running the real project firmware
- `Board B`: second ESP32 monitor running the `INA219` firmware
- laptop running the local MQTT broker and listener
- `INA219` inline current / power monitor
- phone camera or screenshot tool for saving meter evidence

Use:

- [`INA219_ENERGY_SETUP.md`](./INA219_ENERGY_SETUP.md)
- [`../firmware/ina219_power_monitor/README.md`](../firmware/ina219_power_monitor/README.md)
- [`../results/reference_repo_power_conditions_2026-04-21.md`](../results/reference_repo_power_conditions_2026-04-21.md)

## Reference-Repo Matching Conditions

To stay aligned with the class examples, run the DUT under these exact conditions:

| Condition | Baseline run | Adaptive run |
| --- | --- | --- |
| DUT firmware | main `esp32_node` app | same main `esp32_node` app |
| Signal profile | `clean_reference` | `clean_reference` |
| Communication path | `MQTT_ONLY` over WiFi | `MQTT_ONLY` over WiFi |
| Initial sampling rate | `50 Hz` | `50 Hz` |
| Adaptive sampling | disabled | enabled |
| Expected steady rate | `50 Hz` | `40 Hz` |
| Window duration | same project window | same project window |
| Run duration | `120 s` preferred | `120 s` preferred |
| Monitor | same `INA219` setup | same `INA219` setup |
| DUT USB cable | disconnected during measurement | disconnected during measurement |

Save both runs even if the measured power difference is small. Some reference repos also observed that continuous FreeRTOS and WiFi activity can dominate the energy profile; that is an acceptable finding as long as the comparison is honest.

Optional extra run:

- add a deep-sleep cycle measurement if the DUT can wake, connect, publish, and sleep reliably
- present it as an additional low-power strategy, not as a replacement for fixed-vs-adaptive sampling

## Firmware Switches To Use

Edit [`../firmware/esp32_node/include/project_config.h`](../firmware/esp32_node/include/project_config.h):

- baseline run:
  `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U`
- adaptive run:
  `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U`

Keep these the same across both runs:

- `PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ = 50.0f`
- `PROJECT_SIGNAL_PROFILE = PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE`
- `PROJECT_COMMUNICATION_MODE = PROJECT_COMMUNICATION_MODE_MQTT_ONLY`
- window duration and broker settings

Recommended extra test setting:

- `PROJECT_ENABLE_BETTER_SERIAL_PLOTTER = 0U`

That keeps serial-print overhead off the DUT during the energy run while preserving the same project logic in both runs.

## Recommended Run Duration

- minimum: `60 seconds`
- better: `120 seconds`

This gives enough windows to smooth out startup noise.

The strongest final submission version is:

- baseline run: `120 s`
- adaptive run: `120 s`
- same MQTT listener setup for both

## Run Order

### 1. Baseline Fixed-Rate Run

1. On `Board A`, set:
   - `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U`
   - `PROJECT_COMMUNICATION_MODE = PROJECT_COMMUNICATION_MODE_MQTT_ONLY`
   - `PROJECT_SIGNAL_PROFILE = PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE`
2. Build and flash `Board A`.
3. Disconnect `Board A` from USB.
4. Upload the monitor firmware to `Board B` from [`../firmware/ina219_power_monitor/`](../firmware/ina219_power_monitor/).
5. Wire the power path:
   - `Board B` USB-fed `5V` pin `-> INA219 VIN+`
   - `INA219 VIN- -> Board A 5V / VIN`
   - common `GND` between `Board B`, `INA219`, and `Board A`
6. Start the MQTT listener and save a dedicated baseline `CSV` and `JSONL`.
7. Start logging the monitor-board serial output to `source/results/summaries/ina219_baseline.tsv`.
8. Run for `120 seconds`.
9. Save:
   - one photo of the wiring
   - one screenshot of the live monitor stream or `BetterSerialPlotter`
   - the listener output
   - the generated `MQTT` summary markdown
   - the generated `INA219` summary markdown

Recommended filenames:

- `results/energy_baseline_run.png`
- `results/summaries/mqtt_run_<date>_baseline_clean.csv`
- `results/summaries/mqtt_summary_<date>_baseline_clean.md`
- `results/summaries/ina219_baseline.tsv`
- `results/summaries/ina219_baseline.md`

### 2. Adaptive Run

1. On `Board A`, set `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U`.
2. Build and flash `Board A` again.
3. Keep the same monitor-board wiring and the same listener setup.
4. Start a separate monitor log:
   `source/results/summaries/ina219_adaptive.tsv`
5. Run for the same duration as the baseline.
6. Save the same types of artifacts.

Recommended filenames:

- `results/energy_adaptive_run.png`
- `results/summaries/mqtt_run_<date>_adaptive_clean.csv`
- `results/summaries/mqtt_summary_<date>_adaptive_clean.md`
- `results/summaries/ina219_adaptive.tsv`
- `results/summaries/ina219_adaptive.md`

### 3. Optional Repeat Runs

If time allows, repeat each mode `3` times and average the measured power or energy value.

## What To Record

For each run, record:

- date and location
- meter type
- run duration
- firmware mode: `baseline` or `adaptive`
- communication mode
- signal profile
- average power or total energy from the meter
- peak current and peak power from WiFi/MQTT activity
- average `sampling_frequency_hz`
- average `listener_latency_us`
- total payload bytes from the summary

From the new `INA219` scripts, the most useful fields to keep are:

- average current
- average power
- integrated energy
- peak power

## Final Comparison Table

The final captured results are saved in:

- [`../results/summaries/ina219_baseline_2026-04-21.md`](../results/summaries/ina219_baseline_2026-04-21.md)
- [`../results/summaries/ina219_adaptive_2026-04-21.md`](../results/summaries/ina219_adaptive_2026-04-21.md)
- [`../results/summaries/ina219_comparison_2026-04-21.md`](../results/summaries/ina219_comparison_2026-04-21.md)
- [`../results/summaries/ina219_deepsleep_2026-04-21.md`](../results/summaries/ina219_deepsleep_2026-04-21.md)
- [`../results/summaries/ina219_three_mode_comparison_2026-04-21.md`](../results/summaries/ina219_three_mode_comparison_2026-04-21.md)

Summary:

| Metric | Baseline `50 Hz` | Adaptive `40 Hz` | Adaptive + deep sleep |
| --- | ---: | ---: | ---: |
| Average power | `553.0000 mW` | `552.6775 mW` | `410.8682 mW` |
| Integrated energy | `18.433238 mWh` | `18.422466 mWh` | `13.632451 mWh` |
| Delta vs baseline | reference | `-0.06%` | `-26.04%` |

For future reruns, copy fresh results into a table like this:

| Metric | Baseline | Adaptive | Notes |
| --- | --- | --- | --- |
| Run duration |  |  |  |
| Average selected sampling frequency |  |  |  |
| Average power / total energy |  |  |  |
| Total payload bytes |  |  |  |
| Average listener latency |  |  |  |
| Average end-to-end latency |  |  |  |

The repo now provides these helpers:

```powershell
python source/results/analyze_ina219_log.py --input source/results/summaries/ina219_baseline.tsv --label baseline --output-json source/results/summaries/ina219_baseline.json --output-md source/results/summaries/ina219_baseline.md
python source/results/analyze_ina219_log.py --input source/results/summaries/ina219_adaptive.tsv --label adaptive --output-json source/results/summaries/ina219_adaptive.json --output-md source/results/summaries/ina219_adaptive.md
python source/results/compare_ina219_runs.py --baseline source/results/summaries/ina219_baseline.json --adaptive source/results/summaries/ina219_adaptive.json --output-md source/results/summaries/ina219_comparison.md
```

## How To Discuss The Results

The expected interpretation for this project is:

- adaptive mode should reduce local sensing / processing work because the steady-state rate drops from `50 Hz` to `40 Hz`
- network volume may not change much because both modes still send one aggregate per window
- any measured energy improvement should therefore be attributed mainly to local computation and sampling activity, not a large drop in MQTT payload count
- if the measured energy difference is small, that is still a valid result, as long as the setup and comparison are honest and identical across both runs
- if WiFi dominates the power profile, explain that clearly; the reference repos also show WiFi connection / transmission spikes as the largest visible power events

## Evidence To Keep

- one photo of the measurement setup: [`../pics/hardware.png`](../pics/hardware.png)
- one adaptive meter screenshot or photo: [`../pics/2026-04-21_ina219_adaptive_betterserialplotter.png`](../pics/2026-04-21_ina219_adaptive_betterserialplotter.png)
- one deep-sleep meter screenshot or photo: [`../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png`](../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png)
- one summary showing the baseline/adaptive numbers: [`../results/summaries/ina219_comparison_2026-04-21.md`](../results/summaries/ina219_comparison_2026-04-21.md)
- one summary showing the optional deep-sleep result: [`../results/summaries/ina219_three_mode_comparison_2026-04-21.md`](../results/summaries/ina219_three_mode_comparison_2026-04-21.md)

The screenshot checklist already reserves these filenames:

- `power_measurement_hardware_setup.jpg`
- `energy_baseline_run.png`
- `energy_adaptive_run.png`

## If No Meter Is Available In A Future Rerun

If direct power measurement is impossible in a future rerun, keep the runbook and note the limitation explicitly in the final report. For the current submission, direct `INA219` measurements are already saved.
