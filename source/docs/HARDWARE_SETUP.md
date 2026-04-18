# Hardware Setup

## Main Board

Primary platform:

- `RUIZHI ESP32 V3 LoRa Board`
- integrated `WiFi`
- integrated `Bluetooth`
- integrated `868 MHz LoRa`
- integrated OLED display

## Primary Implementation Mode

The main assignment path uses a `virtual sensor`, so no external sensor is required for the first implementation milestones.

This is intentional because:

- it reduces project risk,
- it makes FFT validation easier,
- it allows precise repeatable experiments.

## Optional HC-SR04 Demo Mode

The `HC-SR04` is only for an additional physical demo mode.

### Wiring Rules

- `VCC` to the correct power rail for the module
- `GND` to board `GND`
- `TRIG` to one ESP32 output pin
- `ECHO` to one ESP32 input pin through a voltage divider or level shifter

### Important Safety Note

Many `HC-SR04` modules drive the `ECHO` line at `5V`.

ESP32 GPIO pins are `3.3V` logic pins.

Do not connect `ECHO` directly to the ESP32 unless you have confirmed the module output is already safe for `3.3V`.

## Optional INA219 Energy Monitor

For the graded energy section, an `INA219` high-side current monitor is a good fit.

Important note for this board:

- the generic `ESP32` example from many tutorials uses `GPIO21` / `GPIO22`
- the `Heltec WiFi LoRa 32 V3` Arduino variant instead defines the external `I2C` pins as:
  - `SDA = GPIO41`
  - `SCL = GPIO42`
- the onboard OLED already uses a separate internal bus on:
  - `SDA_OLED = GPIO17`
  - `SCL_OLED = GPIO18`

This means an external `INA219` can be attached to the normal `Wire` bus without conflicting with the OLED bus.

Recommended reference:

- [`INA219_ENERGY_SETUP.md`](./INA219_ENERGY_SETUP.md)

## Suggested Demo Use

The most sensible use of the `HC-SR04` in this project is:

- demonstrate that the board can read a real physical sensor,
- show values on serial output or OLED,
- keep the formal FFT and adaptive-sampling evaluation on the virtual signal path.

## LoRaWAN Test Note

The board can only reach `TTN` when it is within range of a working `LoRaWAN` gateway.

Practical implication for this project:

- local development can validate the full `MQTT/WiFi` path from home,
- the real `LoRaWAN + TTN` uplink test should be done on campus or anywhere else with known gateway coverage,
- final screenshots or logs for the cloud path should be captured during that live gateway test.
