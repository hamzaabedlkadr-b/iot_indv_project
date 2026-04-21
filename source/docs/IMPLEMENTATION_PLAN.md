# Implementation Plan

This is the working step-by-step plan for the project. Each step has a clear goal, tasks, and a concrete output so we can move through the project in order.

## Current Status Snapshot

- Phases `1` to `8` are implemented locally, and the `MQTT/WiFi` edge path has been validated on the real Heltec board.
- Phase `9` is validated: the integrated main app produced live `LoRaWAN/TTN` uplinks with saved serial and TTN screenshots.
- Phase `10` is mostly complete: the repo contains listener tooling, analyzers, MQTT latency/payload datasets, INA219 energy runs, communication-volume comparison, and anomaly-filter evaluation.
- Phase `11` remains optional.
- Phase `12` is in progress through the updated READMEs, runtime notes, and measurement templates.

## Phase 0: Freeze the Project Scope

### Goal

Lock the implementation choices before coding.

### Decisions

- firmware platform: `ESP-IDF`
- task model: native `FreeRTOS`
- primary signal source: `virtual sensor`
- optional real sensor mode: `HC-SR04`
- edge communication: `MQTT over WiFi`
- cloud communication: `LoRaWAN + TTN`
- aggregate function: windowed `average`

### Output

- clear architecture
- stable folder structure
- no ambiguity about the first implementation steps

## Phase 1: Prepare the Development Environment

### Goal

Make sure the board and local machine are ready for development.

### Tasks

1. Install `ESP-IDF`.
2. Confirm the ESP32 board can be detected and flashed.
3. Decide which serial port is used by the board.
4. Install an `MQTT` broker on the nearby server or local machine.
5. Create a simple logging setup for received MQTT messages.

### Done When

- the board flashes successfully,
- serial logs are visible,
- the MQTT broker is reachable over WiFi.

## Phase 2: Build the Firmware Skeleton

### Goal

Create the basic application structure before adding logic.

### Tasks

1. Create the main firmware entry point.
2. Add separate modules for:
   signal input,
   signal processing,
   sampling control,
   MQTT communication,
   LoRaWAN communication,
   metrics,
   OLED status output.
3. Define shared data structures for:
   raw sample,
   FFT result,
   aggregate result,
   transmission payload,
   timing metrics.
4. Decide how tasks communicate:
   queues, shared buffers, or event groups.

### Done When

- firmware modules are separated by responsibility,
- the application boots with placeholder tasks,
- serial logs show task startup clearly.

## Phase 3: Implement the Virtual Sensor

### Goal

Generate a known periodic signal directly on the ESP32.

### Tasks

1. Implement a time-based sinusoidal generator.
2. Start with:
   `2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)`
3. Add a configurable sampling timer.
4. Store samples in a window buffer.
5. Print a small subset of values for validation.

### Done When

- the board produces stable synthetic samples,
- the signal window fills correctly,
- the generated values match expectations.

## Phase 4: Measure Maximum Sampling Frequency

### Goal

Determine the highest stable sampling frequency the hardware can sustain.

### Tasks

1. Start with a safe low frequency.
2. Increase sampling frequency gradually.
3. Measure:
   missed deadlines,
   queue overflow,
   CPU load symptoms,
   sample timing stability.
4. Record the highest stable value.

### Done When

- the maximum stable sampling frequency is measured on the real board,
- the result is written down for later comparison.

## Phase 5: Add FFT-Based Frequency Analysis

### Goal

Estimate the dominant frequency of the signal from each window.

### Tasks

1. Choose an FFT library or implementation suitable for ESP32.
2. Run FFT on the current sample window.
3. Convert FFT bins to actual frequencies.
4. Detect the dominant frequency.
5. Compare the detected frequency with the known virtual input.

### Done When

- the detected dominant frequency is close to the expected input frequency,
- the FFT output is reliable across repeated windows.

## Phase 6: Implement Adaptive Sampling

### Goal

Use the FFT result to reduce unnecessary oversampling.

### Tasks

1. Define an adaptation rule:
   new sampling rate = function of dominant frequency.
2. Enforce a minimum and maximum rate.
3. Update the sampling task dynamically.
4. Log every adaptation decision.
5. Compare adaptive behavior against the fixed oversampled baseline.

### Done When

- the sampling rate changes correctly,
- the adaptive logic remains stable,
- the system still captures the signal correctly.

## Phase 7: Compute the Aggregate Function

### Goal

Compute the assignment's required per-window aggregate.

### Tasks

1. Compute the average over each window.
2. Store:
   window start time,
   window end time,
   sample count,
   sampling frequency,
   dominant frequency,
   average value.
3. Prepare the aggregate result for transmission.

### Done When

- one aggregate result is produced per completed window,
- the average is numerically correct.

## Phase 8: Send Data to the Edge Server

### Goal

Transmit the aggregate to a nearby server over `MQTT` and `WiFi`.

### Tasks

1. Connect the board to WiFi.
2. Connect the firmware to the MQTT broker.
3. Publish one message per window.
4. Store messages on the server side.
5. Add timestamps for latency measurement.

### Done When

- the edge server receives every aggregate,
- payloads are readable and logged,
- timestamps are usable for end-to-end latency analysis.

## Phase 9: Send Data to the Cloud with LoRaWAN

### Goal

Send the aggregate to `TTN`.

### Tasks

1. Set up LoRaWAN credentials.
2. Define a compact uplink payload.
3. Transmit one aggregate per suitable window or interval.
4. Decode payload fields on the cloud side.
5. Save logs for proof of transmission.

### Done When

- TTN receives uplinks from the device,
- the aggregate can be decoded and shown in cloud logs.

## Phase 10: Measure Performance

### Goal

Produce the data needed for the report and final evaluation.

### Tasks

1. Measure energy for:
   fixed oversampling,
   adaptive sampling.
2. Measure per-window execution time.
3. Measure transmitted data volume.
4. Measure end-to-end latency to the edge server.
5. Repeat tests across multiple runs.

### Done When

- there is a usable dataset for comparison,
- plots or tables can be generated from the collected numbers.

## Phase 11: Add Optional HC-SR04 Demo Mode

### Goal

Provide a physical sensor mode without changing the main evaluation path.

### Tasks

1. Wire the `HC-SR04` safely:
   `TRIG` to an ESP32 output,
   `ECHO` to an ESP32 input through a voltage divider or level shifter,
   shared `GND`.
2. Read distance periodically.
3. Add a separate sensor-input mode for `HC-SR04`.
4. Display measured distance on serial and optionally OLED.
5. Keep this mode isolated from the main virtual-sensor experiments.

### Done When

- the board reads distance values correctly,
- the sensor demo works as an optional extension.

## Phase 12: Final Documentation

### Goal

Prepare the repository for submission.

### Tasks

1. Write a clear top-level `README.md`.
2. Document setup and run steps.
3. Add architecture diagrams or flow figures.
4. Add results tables and discussion.
5. Link the saved evidence artifacts for each grading item.

### Done When

- another person can clone the repo and understand the whole system,
- the repository can be used directly during the class workshop.

## Recommended Execution Order

This is the order we should actually follow:

1. Environment setup
2. Firmware skeleton
3. Virtual sensor
4. Maximum sampling frequency measurement
5. FFT
6. Adaptive sampling
7. Aggregate computation
8. MQTT edge path
9. LoRaWAN cloud path
10. Measurements
11. Optional HC-SR04 mode
12. Final report and cleanup

## Immediate Next Task

The next implementation task should be:

`collect the remaining measurement evidence and validate the LoRaWAN uplink on campus`
