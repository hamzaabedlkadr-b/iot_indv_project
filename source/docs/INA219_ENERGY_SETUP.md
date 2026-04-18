# INA219 Energy Setup For Heltec V3

This note adapts the professor's `INA219` example to the `Heltec WiFi LoRa 32 V3` used in this project.

## Why INA219 Fits This Project

The `INA219` can report:

- bus voltage
- shunt voltage
- current
- power

That makes it a practical way to compare:

- fixed baseline sampling
- adaptive sampling

for the same firmware and signal profile.

## Important Heltec V3 Pin Difference

The common tutorial wiring `SDA=21` and `SCL=22` is for a generic ESP32 board.

For the `Heltec WiFi LoRa 32 V3`, the local PlatformIO board variant defines:

- external `I2C` bus:
  - `SDA = GPIO41`
  - `SCL = GPIO42`
- onboard OLED bus:
  - `SDA_OLED = GPIO17`
  - `SCL_OLED = GPIO18`

So for an external `INA219`, use:

| INA219 pin | Heltec V3 pin | Notes |
| --- | --- | --- |
| `VCC` | `3.3V` | use `3.3V` logic |
| `GND` | `GND` | common ground |
| `SDA` | `GPIO41` | external I2C bus |
| `SCL` | `GPIO42` | external I2C bus |

## Power Path Wiring

The `INA219` must sit in series with the positive supply rail of the board under test.

The high-side path is:

| Connection | Direction |
| --- | --- |
| external power source `+` | `-> INA219 VIN+` |
| `INA219 VIN-` | `-> Heltec positive power input` |
| external power source `-` | `-> Heltec GND` |
| external power source `-` | `-> INA219 GND` |

The logic connections for measurement are separate:

| INA219 pin | Heltec V3 pin |
| --- | --- |
| `VCC` | `3.3V` |
| `GND` | `GND` |
| `SDA` | `GPIO41` |
| `SCL` | `GPIO42` |

## The Most Important Practical Catch

If the Heltec is powered directly from `USB`, the `INA219` may not be measuring the full board current at all, because the board is being powered through the USB path instead of through the sensor.

That means:

- `USB power from the PC can bypass the INA219`
- serial-over-USB is convenient, but it can ruin the measurement setup if the board under test is still being powered from the cable

## Best Measurement Setups

### Best Setup: Two Boards

This is the cleanest version if later you have two LoRa / ESP32 boards.

Use:

- `Board A` = device under test running the real project firmware
- `Board B` = monitor board reading the `INA219` and streaming the measurements to the PC

How it works:

1. Power `Board A` only through the `INA219` high-side path.
2. Connect the `INA219` I2C pins to `Board B`.
3. Keep `Board B` on USB to the PC for logging / BetterSerialPlotter.
4. Do not let `Board A` be powered directly from the PC USB during the measurement run.

Why this is best:

- no USB power bypass on the board under test
- no self-measurement overhead mixed into the result
- easier live plotting with `BetterSerialPlotter`

### Usable Setup Now: One Heltec + One INA219

You can still do a relative comparison now with one board, but it is less clean.

The same Heltec:

- runs the project firmware
- reads the `INA219`
- becomes part of the measurement loop

This is acceptable for a baseline-vs-adaptive comparison if:

- the wiring is identical in both runs
- the measurement code stays identical in both runs
- you document the limitation honestly

Main limitation:

- the board is partially measuring itself, so the monitor overhead is included

## Recommendation For Your Current Situation

Since you currently have:

- one LoRa board
- one `INA219`

the safest plan is:

1. Keep the repo and firmware ready now.
2. Use the one-board setup only if you can avoid USB power bypass.
3. When you have two boards, switch to the two-board setup for the final energy evidence.

## How To Use It In This Project

For the actual comparison, pair the `INA219` setup with the firmware switch already added in `project_config.h`:

- `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 0U` for the fixed `50 Hz` baseline
- `PROJECT_ENABLE_ADAPTIVE_SAMPLING = 1U` for the adaptive run

Suggested experiment order:

1. flash baseline mode
2. record `INA219` current / power for `60-120 s`
3. flash adaptive mode
4. record `INA219` current / power for the same duration
5. compare average power and total energy

## What To Plot

If you stream the `INA219` values to `BetterSerialPlotter`, the most useful series are:

- `current_mA`
- `power_mW`
- optionally `busvoltage`

The professor's example already prints values in a plotter-friendly tab-separated format, so that part is aligned with the tooling you already installed.

## What To Save As Evidence

- one photo or diagram of the final wiring
- one screenshot of the live `BetterSerialPlotter` or serial stream
- one baseline energy screenshot
- one adaptive energy screenshot
- one comparison table in the final README

## Recommended Report Wording

If you use the one-board setup now, describe it honestly like this:

`The INA219 was used to compare baseline and adaptive runs under identical conditions. Because only one Heltec board was available during the first measurements, the device under test also acted as the INA219 reader, so the results are most reliable as a relative comparison rather than as a fully isolated absolute power benchmark.`
