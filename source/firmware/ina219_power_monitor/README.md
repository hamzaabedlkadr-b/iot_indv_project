# INA219 Power Monitor Firmware

This folder contains the firmware for the second ESP32 that acts as the `INA219` monitor during the graded energy test.

## Purpose

Use this firmware on the monitor board while:

- `Board A` = the device under test running the real project firmware
- `Board B` = the monitor board connected to the `INA219` and to the PC over USB

This is the same general measurement pattern used in the stronger reference repositories shared by the instructor.

## Recommended Graded Test

For this project, the required energy comparison should be:

1. fixed baseline run at `50 Hz`
2. adaptive run that settles to `40 Hz`

Keep these identical across both runs:

- same board under test
- same `clean_reference` signal profile
- same communication path
- same run duration

For this repo, the cleanest graded setup is:

- `PROJECT_COMMUNICATION_MODE = PROJECT_COMMUNICATION_MODE_MQTT_ONLY`
- `PROJECT_SIGNAL_PROFILE = PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE`
- baseline run with `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U`
- adaptive run with `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U`

Do not use the deep-sleep example as the main graded comparison unless you explicitly want it as an extra discussion point. It is not the core adaptive-sampling comparison for this repository.

## Wiring

### INA219 Logic Wiring To The Monitor Board

If the monitor board is a generic `ESP32 DevKit`:

| INA219 pin | Monitor ESP32 pin |
| --- | --- |
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SDA` | `GPIO21` |
| `SCL` | `GPIO22` |

If the monitor board is a `Heltec WiFi LoRa 32 V3`:

| INA219 pin | Monitor Heltec pin |
| --- | --- |
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SDA` | `GPIO41` |
| `SCL` | `GPIO42` |

### High-Side Power Path Without A Separate Bench Supply

If you do not have a separate power supply, use the monitor board's USB-fed `5V` pin as the source.

The power path becomes:

| Connection | Direction |
| --- | --- |
| Monitor board `5V` / `VIN` / `VU` / `VBUS` pin | `-> INA219 VIN+` |
| `INA219 VIN-` | `-> DUT board 5V / VIN input` |
| Monitor board `GND` | `-> INA219 GND` |
| Monitor board `GND` | `-> DUT GND` |

Important:

- use the monitor board's USB-fed `5V` pin to power the DUT
- do not use the monitor board `3.3V` pin to power the DUT
- keep the DUT USB disconnected during the measurement run, otherwise USB power can bypass the `INA219`

If the DUT browns out during WiFi activity, move to a powered USB hub or a separate `5V` supply later. For a relative baseline-vs-adaptive comparison, the USB-fed `5V` path is still acceptable if it remains stable in both runs.

## Build And Upload

Generic monitor ESP32:

```powershell
pio run -d source/firmware/ina219_power_monitor -e esp32dev_monitor -t upload
```

Heltec V3 as the monitor board:

```powershell
pio run -d source/firmware/ina219_power_monitor -e heltec_v3_monitor -t upload
```

## Output Format

The monitor prints tab-separated columns:

```text
elapsed_ms    bus_voltage_V    shunt_voltage_mV    load_voltage_V    current_mA    power_mW
```

That format works for:

- raw file capture
- quick inspection in a serial terminal
- plotting in `BetterSerialPlotter`
- post-processing with [`../../results/analyze_ina219_log.py`](../../results/analyze_ina219_log.py)

## Suggested Capture Command

Keep the monitor board connected to the PC over USB and save the output:

```powershell
pio device monitor -d source/firmware/ina219_power_monitor -e esp32dev_monitor --raw > source/results/summaries/ina219_baseline.tsv
```

Replace the output filename for the adaptive run, for example:

- `source/results/summaries/ina219_baseline.tsv`
- `source/results/summaries/ina219_adaptive.tsv`

## Post-Processing

After each run:

```powershell
python source/results/analyze_ina219_log.py --input source/results/summaries/ina219_baseline.tsv --label baseline --output-json source/results/summaries/ina219_baseline.json --output-md source/results/summaries/ina219_baseline.md
```

```powershell
python source/results/analyze_ina219_log.py --input source/results/summaries/ina219_adaptive.tsv --label adaptive --output-json source/results/summaries/ina219_adaptive.json --output-md source/results/summaries/ina219_adaptive.md
```

Then compare them:

```powershell
python source/results/compare_ina219_runs.py --baseline source/results/summaries/ina219_baseline.json --adaptive source/results/summaries/ina219_adaptive.json --output-md source/results/summaries/ina219_comparison.md
```
