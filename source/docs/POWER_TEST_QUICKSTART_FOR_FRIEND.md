# Power Test Quickstart

This is the short guide for rerunning the `INA219` power-consumption test.

## Where Things Are

- DUT firmware: [`../firmware/esp32_node/`](../firmware/esp32_node/)
- INA219 monitor firmware: [`../firmware/ina219_power_monitor/`](../firmware/ina219_power_monitor/)
- Main config: [`../firmware/esp32_node/include/project_config.h`](../firmware/esp32_node/include/project_config.h)
- Local private config: `source/firmware/esp32_node/include/project_config_local.h`
- Result summaries: [`../results/summaries/`](../results/summaries/)
- Screenshots/photos: [`../pics/`](../pics/)
- Final evidence map: [`../results/final_evidence_index_2026-04-21.md`](../results/final_evidence_index_2026-04-21.md)

## Wiring

INA219 to monitor Heltec:

| INA219 | Monitor Heltec |
| --- | --- |
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SDA` | `GPIO41` |
| `SCL` | `GPIO42` |

Power path:

| Connection | Direction |
| --- | --- |
| Monitor `5V` / `VIN` / `VBUS` | `-> INA219 VIN+` |
| `INA219 VIN-` | `-> DUT 5V` / `VIN` |
| Monitor `GND` | `-> DUT GND` |

Important: disconnect DUT USB during measurement, so all DUT power goes through the `INA219`.

## Upload Monitor Code

```powershell
pio run -d source\firmware\ina219_power_monitor -e heltec_v3_monitor -t upload
```

Use `-e esp32dev_monitor` if the monitor board is a generic ESP32.

## Run 1: Baseline `50 Hz`

In `project_config_local.h`, set:

```c
#undef PROJECT_COMMUNICATION_MODE
#define PROJECT_COMMUNICATION_MODE PROJECT_COMMUNICATION_MODE_MQTT_ONLY

#undef PROJECT_SIGNAL_PROFILE
#define PROJECT_SIGNAL_PROFILE PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE

#undef PROJECT_ENABLE_ADAPTIVE_SAMPLING
#define PROJECT_ENABLE_ADAPTIVE_SAMPLING 0U

#undef PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT
#define PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT 0U
```

Upload DUT:

```powershell
pio run -d source\firmware\esp32_node -e heltec_wifi_lora_32_V3 -t upload
```

Disconnect DUT USB, power it through `INA219`, then capture about `120 s`:

```powershell
pio device monitor -d source\firmware\ina219_power_monitor -e heltec_v3_monitor --raw > source\results\summaries\friend_ina219_baseline.tsv
```

## Run 2: Adaptive `40 Hz`

Change only:

```c
#undef PROJECT_ENABLE_ADAPTIVE_SAMPLING
#define PROJECT_ENABLE_ADAPTIVE_SAMPLING 1U
```

Upload DUT again, disconnect DUT USB, power it through `INA219`, then capture:

```powershell
pio device monitor -d source\firmware\ina219_power_monitor -e heltec_v3_monitor --raw > source\results\summaries\friend_ina219_adaptive.tsv
```

## Analyze

```powershell
python source\results\analyze_ina219_log.py --input source\results\summaries\friend_ina219_baseline.tsv --label baseline --output-json source\results\summaries\friend_ina219_baseline.json --output-md source\results\summaries\friend_ina219_baseline.md
```

```powershell
python source\results\analyze_ina219_log.py --input source\results\summaries\friend_ina219_adaptive.tsv --label adaptive --output-json source\results\summaries\friend_ina219_adaptive.json --output-md source\results\summaries\friend_ina219_adaptive.md
```

```powershell
python source\results\compare_ina219_runs.py --baseline source\results\summaries\friend_ina219_baseline.json --adaptive source\results\summaries\friend_ina219_adaptive.json --output-md source\results\summaries\friend_ina219_comparison.md
```

## Send Back

- `friend_ina219_baseline.tsv`
- `friend_ina219_adaptive.tsv`
- `friend_ina219_comparison.md`
- one hardware wiring photo
- one screenshot of the baseline plot
- one screenshot of the adaptive plot

## Common Fixes

- `COM port access denied`: close BetterSerialPlotter / serial monitor / PlatformIO monitor.
- `Failed to find INA219`: check `VCC`, `GND`, `SDA`, `SCL`, then reset monitor.
- DUT resets: use a stronger `5V` source or powered USB hub.
- Bad readings: make sure DUT USB is disconnected and DUT power goes through `VIN+ -> VIN-`.
