# BetterSerialPlotter Guide

This project can stream a BetterSerialPlotter-friendly numeric output over USB serial.

Tool:

- <https://github.com/nathandunk/BetterSerialPlotter>

## What This Is For

Use this mode when you want a cleaner live visualization of the adaptive-sampling pipeline, for example:

- sampling frequency over time
- dominant frequency over time
- aggregate average over time
- anomaly count over time

This mode is mainly useful for:

- workshop explanation
- screenshots for the final README
- quick visual sanity checks

It is not a replacement for the saved MQTT evidence and measurement summaries.

## Current Firmware Setting

The serial plotter stream is controlled in [`project_config.h`](../firmware/esp32_node/include/project_config.h):

- `PROJECT_ENABLE_BETTER_SERIAL_PLOTTER`
- `PROJECT_BETTER_SERIAL_PLOTTER_SILENCE_INFO_LOGS`

When plotter mode is enabled:

- the firmware emits one numeric row per completed aggregate window
- normal `ESP_LOGI(...)` output is silenced to keep the stream clean
- warnings and errors may still appear if something goes wrong

## Column Order

Each numeric row is printed in this order:

1. `sampling_frequency_hz`
2. `dominant_frequency_hz`
3. `average_value`

Example row:

```text
40.00    5.00    -0.0001
```

BetterSerialPlotter uses its own wall-clock x-axis, so we do not need to send a separate time column.

## How To Use It

1. Flash the firmware with plotter mode enabled.
2. Open BetterSerialPlotter.
3. Select the board serial port.
4. Set the baud rate to match the firmware serial output.
5. Wait for the first complete window to be produced.
6. Add the variables you want to plot.

Recommended plots:

- `sampling_frequency_hz`
- `dominant_frequency_hz`
- `average_value`

These will appear as `data 0`, `data 1`, and `data 2` unless you rename them in BetterSerialPlotter.

## Recommended Screenshot Targets

If we use BetterSerialPlotter for evidence, the best screenshots are:

- adaptive sampling settling from the initial rate to the lower selected rate
- clean vs noisy vs anomaly profile comparison
- anomaly count becoming non-zero in the anomaly profile

## When To Turn It Off

Disable plotter mode when you want:

- the full human-readable serial logs again
- easier debugging of WiFi, MQTT, or LoRaWAN runtime details

In that case, set:

```c
#define PROJECT_ENABLE_BETTER_SERIAL_PLOTTER 0U
```

and rebuild the firmware.
