# Power Consumption Test Quickstart

This guide is for someone helping rerun the `INA219` power-consumption test.
It is intentionally short and practical.

Full reference docs:

- [`ENERGY_MEASUREMENT_RUNBOOK.md`](./ENERGY_MEASUREMENT_RUNBOOK.md)
- [`INA219_ENERGY_SETUP.md`](./INA219_ENERGY_SETUP.md)
- [`../firmware/ina219_power_monitor/README.md`](../firmware/ina219_power_monitor/README.md)

## Where The Code And Results Are

- Main DUT firmware code: [`../firmware/esp32_node/`](../firmware/esp32_node/). This is the code uploaded to the Heltec that runs the virtual signal, FFT, adaptive sampling, MQTT, and LoRaWAN logic.
- Monitor firmware code: [`../firmware/ina219_power_monitor/`](../firmware/ina219_power_monitor/). This is the code uploaded to the second ESP32/Heltec connected to the `INA219`.
- Main configuration file: [`../firmware/esp32_node/include/project_config.h`](../firmware/esp32_node/include/project_config.h). Local WiFi/MQTT secrets go in ignored `project_config_local.h`.
- Saved result summaries: [`../results/summaries/`](../results/summaries/). Check here for `ina219_*`, `mqtt_*`, anomaly-filter, and communication-volume results.
- Saved screenshots/photos: [`../pics/`](../pics/). Check here for hardware photos, BetterSerialPlotter screenshots, MQTT proof, and TTN proof.
- Final evidence map: [`../results/final_evidence_index_2026-04-21.md`](../results/final_evidence_index_2026-04-21.md). This is the best single file to know which result proves which assignment requirement.

## Goal

Measure the same DUT under the same conditions in two modes:

| Run | DUT mode | Expected sampling behavior |
| --- | --- | --- |
| Baseline | adaptive sampling disabled | fixed `50 Hz` |
| Adaptive | adaptive sampling enabled | starts at `50 Hz`, then settles to `40 Hz` |

Optional extra:

- run adaptive + deep sleep as a separate low-power comparison, not as the main required comparison.

## Hardware Roles

| Name | Board | Job |
| --- | --- | --- |
| DUT / Board A | Heltec WiFi LoRa 32 V3 | runs the main project firmware |
| Monitor / Board B | Heltec V3 or generic ESP32 | reads the `INA219` and stays connected to the PC |
| INA219 | current/power monitor | sits in series with the DUT `5V` power path |

Important:

- The DUT USB cable must be disconnected during measurement.
- The DUT must receive power only through `INA219 VIN-`.
- The monitor board stays connected to USB because it powers the `INA219` logic and prints measurements.

## Wiring

### INA219 Logic To Monitor Board

For a `Heltec WiFi LoRa 32 V3` monitor:

| INA219 pin | Monitor Heltec pin |
| --- | --- |
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SDA` | `GPIO41` |
| `SCL` | `GPIO42` |

For a generic ESP32 monitor:

| INA219 pin | Monitor ESP32 pin |
| --- | --- |
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SDA` | `GPIO21` |
| `SCL` | `GPIO22` |

### DUT Power Path

If there is no bench power supply, use the monitor board USB `5V`/`VIN`/`VBUS` pin as the source:

| Connection | Direction |
| --- | --- |
| Monitor `5V` / `VIN` / `VBUS` | `-> INA219 VIN+` |
| `INA219 VIN-` | `-> DUT 5V` / `VIN` |
| Monitor `GND` | `-> INA219 GND` |
| Monitor `GND` | `-> DUT GND` |

Do not power the DUT from `3.3V`. Use the `5V`/`VIN` path.

## Before Running

Clone the repo:

```powershell
git clone https://github.com/hamzaabedlkadr-b/iot_indv_project.git
cd iot_indv_project
```

Install PlatformIO if needed, then copy local config:

```powershell
copy source\firmware\esp32_node\include\project_config_local.example.h source\firmware\esp32_node\include\project_config_local.h
```

Edit `source/firmware/esp32_node/include/project_config_local.h` with local WiFi and MQTT broker values.
Do not commit this file.

## Run 1: Baseline Fixed `50 Hz`

In `project_config_local.h`, set:

```c
#undef PROJECT_COMMUNICATION_MODE
#define PROJECT_COMMUNICATION_MODE PROJECT_COMMUNICATION_MODE_MQTT_ONLY

#undef PROJECT_SIGNAL_PROFILE
#define PROJECT_SIGNAL_PROFILE PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE

#undef PROJECT_ENABLE_BETTER_SERIAL_PLOTTER
#define PROJECT_ENABLE_BETTER_SERIAL_PLOTTER 0U

#undef PROJECT_ENABLE_ADAPTIVE_SAMPLING
#define PROJECT_ENABLE_ADAPTIVE_SAMPLING 0U

#undef PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT
#define PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT 0U
```

Upload to the DUT:

```powershell
pio run -d source\firmware\esp32_node -e heltec_wifi_lora_32_V3 -t upload
```

Disconnect DUT USB.

Upload the monitor firmware to the monitor board:

```powershell
pio run -d source\firmware\ina219_power_monitor -e heltec_v3_monitor -t upload
```

Use `-e esp32dev_monitor` instead if the monitor is a generic ESP32 DevKit.

With the DUT powered through `INA219`, capture about `120 s` of monitor output:

```powershell
pio device monitor -d source\firmware\ina219_power_monitor -e heltec_v3_monitor --raw > source\results\summaries\friend_ina219_baseline.tsv
```

Stop with `Ctrl+C` after about `120 s`.

## Run 2: Adaptive `50 Hz -> 40 Hz`

Reconnect DUT USB, change only this line in `project_config_local.h`:

```c
#undef PROJECT_ENABLE_ADAPTIVE_SAMPLING
#define PROJECT_ENABLE_ADAPTIVE_SAMPLING 1U
```

Upload to the DUT again:

```powershell
pio run -d source\firmware\esp32_node -e heltec_wifi_lora_32_V3 -t upload
```

Disconnect DUT USB and reconnect DUT power through `INA219`.

Capture another `120 s`:

```powershell
pio device monitor -d source\firmware\ina219_power_monitor -e heltec_v3_monitor --raw > source\results\summaries\friend_ina219_adaptive.tsv
```

## Optional Run 3: Adaptive + Deep Sleep

Only do this after the first two runs are saved.

Set:

```c
#undef PROJECT_ENABLE_ADAPTIVE_SAMPLING
#define PROJECT_ENABLE_ADAPTIVE_SAMPLING 1U

#undef PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT
#define PROJECT_ENABLE_DEEP_SLEEP_EXPERIMENT 1U
```

Upload the DUT, disconnect DUT USB, and capture:

```powershell
pio device monitor -d source\firmware\ina219_power_monitor -e heltec_v3_monitor --raw > source\results\summaries\friend_ina219_deepsleep.tsv
```

## Analyze Results

After the baseline run:

```powershell
python source\results\analyze_ina219_log.py --input source\results\summaries\friend_ina219_baseline.tsv --label friend_baseline --output-json source\results\summaries\friend_ina219_baseline.json --output-md source\results\summaries\friend_ina219_baseline.md
```

After the adaptive run:

```powershell
python source\results\analyze_ina219_log.py --input source\results\summaries\friend_ina219_adaptive.tsv --label friend_adaptive --output-json source\results\summaries\friend_ina219_adaptive.json --output-md source\results\summaries\friend_ina219_adaptive.md
```

Compare:

```powershell
python source\results\compare_ina219_runs.py --baseline source\results\summaries\friend_ina219_baseline.json --adaptive source\results\summaries\friend_ina219_adaptive.json --output-md source\results\summaries\friend_ina219_comparison.md
```

## What Screenshots / Photos To Save

Save these:

1. Hardware wiring photo showing monitor board, INA219, and DUT power path.
2. Baseline plot or terminal output showing power/current for `120 s`.
3. Adaptive plot or terminal output showing power/current for `120 s`.
4. Optional deep-sleep plot if run.
5. Generated comparison markdown or terminal output showing average power and integrated energy.

Good filenames:

```text
source/pics/friend_power_hardware.jpg
source/pics/friend_power_baseline.png
source/pics/friend_power_adaptive.png
source/pics/friend_power_deepsleep.png
```

## Expected Interpretation

The awake baseline/adaptive difference may be small. That is acceptable.

Why:

- adaptive sampling reduces local sample-processing work from `50 Hz` to `40 Hz`
- but WiFi, display, MQTT, and always-on FreeRTOS tasks dominate power
- optional deep sleep usually gives a much bigger reduction because the whole board sleeps

In the current saved project run:

| Mode | Average power | Integrated energy |
| --- | ---: | ---: |
| Baseline `50 Hz` | `553.0000 mW` | `18.433238 mWh` |
| Adaptive `40 Hz` | `552.6775 mW` | `18.422466 mWh` |
| Adaptive + deep sleep | `410.8682 mW` | `13.632451 mWh` |

## Troubleshooting

| Symptom | Fix |
| --- | --- |
| `Failed to find INA219` | Check `VCC`, `GND`, `SDA`, `SCL`; reset monitor board; confirm correct monitor environment |
| `COM port access denied` | Close BetterSerialPlotter, PlatformIO monitor, Arduino serial monitor, or any other serial app |
| DUT not powering | Check `5V -> VIN+ -> VIN- -> DUT 5V`; ensure common ground |
| `VIN-` much lower than `VIN+` with no load | Check screw terminal contact and wiring; `VIN+`/`VIN-` should be nearly the same with no load |
| DUT resets during WiFi | USB source may not supply enough current; use powered USB hub or stable `5V` source |
| Weird I2C behavior | Do not place LEDs or other loads on `SDA`/`SCL` during the real test |
