# Reference Repo Power-Test Conditions - 2026-04-21

This note summarizes how the instructor-shared reference repositories treat the energy / power-consumption part, so this project can follow the same style without overclaiming.

## Reference Patterns

### `albertoDonof/IoT-Individual-Assignment`

Energy test style:

- used a second ESP32 board as the monitor
- compared multiple runs:
  - average computation with delay between computations and MQTT sending
  - original over-sampled frequency
  - deep sleep after sending the aggregate
- observed WiFi / MQTT connection and publish spikes around the high-power portions
- noted that oversampling and adaptive behavior can show similar power if the same FreeRTOS tasks keep running continuously

Relevant reference:

- <https://github.com/albertoDonof/IoT-Individual-Assignment>

### `afk-codze/heltec-lora-signal-analytics`

Energy test style:

- compared LoRaWAN mode and WiFi mode separately
- for WiFi mode, used two Heltec LoRa 32 V3 boards:
  - one DUT running FreeRTOS signal sampling and transmission
  - one monitor board connected to an INA219 sensor
- measured current while the DUT generated a composite sine wave, performed FFT learning, entered adaptive sampling, and uploaded via WiFi/MQTT
- explicitly discussed WiFi power-saving behavior and the effect of disabling WiFi power save with `esp_wifi_set_ps(WIFI_PS_NONE)`

Relevant reference:

- <https://github.com/afk-codze/heltec-lora-signal-analytics>

### `MarceloJimenez/IoTSignalProcessing`

Energy test style:

- presented qualitative and quantitative energy captures
- separated wake/reconnect, default sampling, and deep-sleep phases
- described deep sleep as an optimization and MQTT frequency reduction as a communication-energy saving

Relevant reference:

- <https://github.com/MarceloJimenez/IoTSignalProcessing>

### `volpeffervescente/IOT-course-individual-assignment`

Energy test style:

- used an ESP32 signal generator / power source, Heltec DUT, and Arduino Mega + INA219 monitor
- measured a single execution cycle with:
  - WiFi + MQTT connection
  - signal sampling
  - FFT computation
  - MQTT publish
  - deep sleep
- showed both current and power curves
- reported phase-level averages / peaks and estimated energy using `E = P * t`

Relevant reference:

- <https://github.com/volpeffervescente/IOT-course-individual-assignment>

## What We Should Do To Match Them

The strongest matching test for this repository is:

1. Use a two-board INA219 setup if possible.
2. Power the DUT only through `INA219 VIN+ -> VIN-`.
3. Run the same main firmware pipeline in both modes.
4. Compare:
   - fixed full-pipeline baseline: `50 Hz`
   - adaptive steady-state mode: `40 Hz`
5. Keep the same:
   - DUT board
   - WiFi network
   - MQTT broker/listener
   - `clean_reference` signal
   - `MQTT_ONLY` communication mode
   - run duration, preferably `120 s`
6. Record:
   - average current
   - average power
   - peak current/power
   - integrated energy
   - number of MQTT windows sent
   - total payload bytes
7. Optionally add a third deep-sleep cycle measurement, but label it as an extra low-power strategy rather than the main adaptive-vs-baseline comparison.

## Current Project Position

The current setup now matches the reference style in both software and evidence:

- main FreeRTOS pipeline is implemented
- fixed/adaptive switch exists
- MQTT listener and summary scripts exist
- INA219 monitor firmware exists
- INA219 analyzer and comparison scripts exist
- two-board INA219 hardware evidence is saved
- fixed `50 Hz`, adaptive `40 Hz`, and optional adaptive + deep-sleep measurements are summarized

Final saved evidence:

- [`summaries/ina219_baseline_2026-04-21.md`](./summaries/ina219_baseline_2026-04-21.md)
- [`summaries/ina219_adaptive_2026-04-21.md`](./summaries/ina219_adaptive_2026-04-21.md)
- [`summaries/ina219_comparison_2026-04-21.md`](./summaries/ina219_comparison_2026-04-21.md)
- [`summaries/ina219_deepsleep_2026-04-21.md`](./summaries/ina219_deepsleep_2026-04-21.md)
- [`summaries/ina219_three_mode_comparison_2026-04-21.md`](./summaries/ina219_three_mode_comparison_2026-04-21.md)
- [`../pics/hardware.png`](../pics/hardware.png)
- [`../pics/2026-04-21_ina219_adaptive_betterserialplotter.png`](../pics/2026-04-21_ina219_adaptive_betterserialplotter.png)
- [`../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png`](../pics/2026-04-21_ina219_deepsleep_betterserialplotter.png)

Key result:

- fixed `50 Hz` baseline: `18.433238 mWh`
- adaptive `40 Hz`: `18.422466 mWh` (`-0.06%`)
- optional adaptive + deep sleep: `13.632451 mWh` (`-26.04%` versus fixed baseline)

This closes the stable hardware measurement item from the original plan. The honest interpretation is that adaptive sampling reduces the local sample-processing workload, but the always-on WiFi/display/FreeRTOS baseline dominates average board power; deep sleep is therefore presented separately as an additional low-power strategy.
