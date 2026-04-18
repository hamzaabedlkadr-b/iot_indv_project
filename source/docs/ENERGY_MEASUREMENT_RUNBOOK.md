# Energy Measurement Runbook

This file turns the remaining energy requirement into a short, repeatable procedure.

If you use the `INA219` current / power monitor mentioned on the course page, also read:

- `docs/INA219_ENERGY_SETUP.md`

## Goal

Compare:

- fixed oversampling baseline at `50 Hz`
- adaptive sampling mode that drops to `40 Hz` for the current clean signal

The comparison should be done with the same board, same signal profile, same communication path, and the same run duration.

## Recommended Setup

- Heltec / ESP32 LoRa board
- laptop running the local MQTT broker and listener
- USB power meter, inline current meter, or another direct power-measurement tool
- phone camera or screenshot tool for saving meter evidence

## Firmware Switches To Use

Edit [`../firmware/esp32_node/include/project_config.h`](../firmware/esp32_node/include/project_config.h):

- baseline run:
  `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U`
- adaptive run:
  `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U`

Keep these the same across both runs:

- `PROJECT_INITIAL_SAMPLING_FREQUENCY_HZ = 50.0f`
- `PROJECT_SIGNAL_PROFILE = PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE`
- `PROJECT_COMMUNICATION_MODE = PROJECT_COMMUNICATION_MODE_MQTT_ONLY` or `BOTH`
- window duration and broker settings

## Recommended Run Duration

- minimum: `60 seconds`
- better: `120 seconds`

This gives enough windows to smooth out startup noise.

## Run Order

### 1. Baseline Fixed-Rate Run

1. Set `PROJECT_ENABLE_ADAPTIVE_SAMPLING` to `0U`.
2. Build and flash the firmware.
3. Start the MQTT listener and save a dedicated baseline `CSV` and `JSONL`.
4. Reset the power meter or note the starting energy value.
5. Run for `60-120 seconds`.
6. Save:
   - a photo or screenshot of the meter
   - the listener output
   - the generated summary markdown

Recommended filenames:

- `results/energy_baseline_run.png`
- `results/summaries/mqtt_run_<date>_baseline_clean.csv`
- `results/summaries/mqtt_summary_<date>_baseline_clean.md`

### 2. Adaptive Run

1. Set `PROJECT_ENABLE_ADAPTIVE_SAMPLING` to `1U`.
2. Build and flash the firmware.
3. Start the MQTT listener and save a separate adaptive `CSV` and `JSONL`.
4. Reset the power meter or note the new starting value.
5. Run for the same duration as the baseline.
6. Save the same types of artifacts.

Recommended filenames:

- `results/energy_adaptive_run.png`
- `results/summaries/mqtt_run_<date>_adaptive_clean.csv`
- `results/summaries/mqtt_summary_<date>_adaptive_clean.md`

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
- average `sampling_frequency_hz`
- average `listener_latency_us`
- total payload bytes from the summary

## Final Comparison Table

Copy the results into a table like this:

| Metric | Baseline | Adaptive | Notes |
| --- | --- | --- | --- |
| Run duration |  |  |  |
| Average selected sampling frequency |  |  |  |
| Average power / total energy |  |  |  |
| Total payload bytes |  |  |  |
| Average listener latency |  |  |  |
| Average end-to-end latency |  |  |  |

## How To Discuss The Results

The expected interpretation for this project is:

- adaptive mode should reduce local sensing / processing work because the steady-state rate drops from `50 Hz` to `40 Hz`
- network volume may not change much because both modes still send one aggregate per window
- any measured energy improvement should therefore be attributed mainly to local computation and sampling activity, not a large drop in MQTT payload count

## Evidence To Keep

- one photo of the measurement setup
- one baseline meter screenshot or photo
- one adaptive meter screenshot or photo
- one summary screenshot showing the baseline numbers
- one summary screenshot showing the adaptive numbers

The screenshot checklist already reserves these filenames:

- `power_measurement_hardware_setup.jpg`
- `energy_baseline_run.png`
- `energy_adaptive_run.png`

## If No Meter Is Available

If direct power measurement is impossible, keep the runbook and note the limitation explicitly in the final report. A direct meter is still the preferred method for the graded energy section.
