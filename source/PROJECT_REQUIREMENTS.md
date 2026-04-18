# IoT Project Requirements

## Project Goal

Build an IoT system on an ESP32-based device running FreeRTOS that:

- collects sensor data,
- analyzes the data locally,
- adapts the sampling frequency to reduce energy and communication overhead,
- computes an aggregated value over a time window,
- sends the aggregate to a nearby edge server and to the cloud,
- evaluates performance in terms of energy, latency, and communication cost.

## Hardware Assumptions

This project should be planned around the hardware already available:

- `RUIZHI ESP32 V3 LoRa Board`
- integrated `WiFi`
- integrated `Bluetooth`
- integrated `868 MHz LoRa` radio
- integrated `0.96"` OLED display

Practical assumption for this project:

- The existing ESP32 LoRa board is the main device platform.
- No extra dedicated sensor hardware is required if a virtual/simulated sensor signal is used.
- This is consistent with the assignment, which allows virtual input signals and mentions the virtual sensor discussed in class.
- A laptop, desktop, or Raspberry Pi can act as the nearby edge server.
- LoRaWAN communication should target `TTN` using the board's `868 MHz` radio configuration when available.

## Functional Requirements

### 1. Input Signal

The system must process an input signal of the form:

`sum(a_k * sin(f_k))`

Example:

`2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)`

The signal may come from:

- a virtual sensor implemented in firmware, or
- a real sensor if one is later added.

### 2. Maximum Sampling Frequency

The system must determine the maximum sampling frequency achievable by the hardware platform.

Expected outcome:

- identify and document the highest stable sampling rate supported by the ESP32-based device,
- demonstrate that the board can sample at a high frequency,
- report the measured value, not just a theoretical one.

### 3. Optimal Sampling Frequency

The system must compute the FFT of the input signal and estimate the dominant frequency content.

Based on that result, the firmware must adapt the sampling frequency accordingly.

Example:

- if the maximum signal frequency is `5 Hz`, the adapted sampling rate may be set to about `10 Hz`.

Expected outcome:

- oversampling is reduced when possible,
- the adapted sampling rate still preserves the relevant signal information.

### 4. Aggregate Function Over a Window

The system must compute an aggregate over a time window.

Minimum required aggregate:

- average value of the sampled signal over a fixed window

Example:

- average over `5 seconds`

### 5. Communication to Nearby Edge Server

The aggregate value must be transmitted to a nearby edge server using:

- `MQTT`
- over `WiFi`

Expected outcome:

- successful local network delivery,
- secure communication if required by the assignment or broker setup.

### 6. Communication to the Cloud

The aggregate value must also be transmitted to a cloud service using:

- `LoRaWAN`
- `TTN`

Expected outcome:

- successful uplink of the aggregate value through the LoRaWAN path,
- documentation of the end-to-end cloud communication flow.

## Performance Evaluation Requirements

The project must measure and discuss the following:

### 1. Energy Savings

Compare energy usage between:

- the original over-sampled configuration,
- the adaptive sampling configuration

Also discuss cases where sleep-policy or timing overhead may reduce the expected savings.

### 2. Per-Window Execution Time

Measure the execution time for each processing window.

This should include at least:

- sampling,
- FFT processing,
- aggregate computation,
- communication-related processing if relevant.

### 3. Network Data Volume

Measure transmitted data volume for:

- the original over-sampled approach,
- the adaptive sampling approach

The result should show whether adaptive sampling reduces communication cost.

### 4. End-to-End Latency

Measure latency from:

- the moment data are generated,
- to the moment the result is received by the edge server

## LLM Usage Requirement

The assignment explicitly requires the use of an LLM.

You must:

- implement the service through a series of prompts,
- report the prompts used,
- comment on the quality of the generated code,
- discuss the opportunities and limitations of using the LLM.

## Bonus Requirements

### 1. Multiple Input Signals

Test at least `3` different input signals and compare performance.

Discuss how different signal types affect adaptive sampling versus basic over-sampling.

### 2. Noisy and Anomalous Signal

In addition to the clean sinusoidal signal, evaluate a signal of the form:

`s(t) = 2*sin(2*pi*3*t) + 4*sin(2*pi*5*t) + n(t) + A(t)`

Where:

- `n(t)` is Gaussian noise with small sigma,
- `A(t)` is a sparse anomaly injection component with occasional large spikes.

### 3. Anomaly-Aware Filtering

Add and compare two filtering approaches:

- `Z-score`
- `Hampel`

Evaluate them using:

- `TPR` (True Positive Rate)
- `FPR` (False Positive Rate)
- mean error reduction across anomaly rates such as `1%`, `5%`, and `10%`
- execution time
- energy impact

### 4. FFT Impact of Anomalies

Measure and discuss:

- how anomalies affect FFT-based dominant frequency estimation,
- the difference between unfiltered and filtered cases,
- the impact on adaptive sampling decisions and energy use.

### 5. Filter Window Trade-Off

Evaluate how filter window size affects:

- computational effort,
- end-to-end delay,
- memory usage

## Submission Requirements

The final submission must include a GitHub repository containing:

- all source code,
- scripts,
- configuration files,
- documentation needed to set up and run the system.

The main `README.md` should include:

- technical explanation of the system,
- how to set up the environment,
- how to run the firmware and server components,
- how measurements were collected,
- results and discussion,
- LLM prompt history and reflection.

## Recommended Project Framing For This Hardware

Given the available board, a realistic project framing is:

- use the `ESP32` as the FreeRTOS execution platform,
- generate the required sinusoidal input in firmware as a virtual sensor,
- run FFT and adaptive sampling logic locally on the board,
- send windowed averages to a local `MQTT` broker over `WiFi`,
- send the same aggregate to `TTN` over `LoRaWAN`,
- use the OLED for optional status/debug output,
- use a PC or Raspberry Pi as the edge server for logging and latency measurement.

This approach satisfies the assignment while avoiding dependency on extra sensor hardware.

## Chosen Implementation Strategy

The project will follow this implementation plan:

- use a `virtual sensor` as the main input source for the required pipeline,
- use the virtual signal for FFT validation, adaptive sampling, aggregation, and performance measurements,
- optionally add one `real sensor` only as an extra physical demo mode,
- use `HC-SR04` as the optional real sensor demo.

Reason for this choice:

- the virtual sensor gives a controlled and repeatable sinusoidal input,
- it is the safest way to satisfy the assignment requirements for FFT and adaptive sampling,
- it avoids blocking the project on extra hardware complexity,
- the `HC-SR04` can still be used later to demonstrate that the platform can acquire real sensor data.

## Optional HC-SR04 Demo Mode

The `HC-SR04` may be integrated as an additional real-sensor mode, but it should not replace the virtual sensor as the main evaluation path.

Recommended use:

- keep the main experiments based on the virtual signal,
- use the `HC-SR04` only to show an optional live sensing demo,
- treat the distance measurements as a secondary feature in the final presentation.

Important hardware note:

- many `HC-SR04` modules use a `5V` `ECHO` signal,
- the ESP32 GPIO pins are `3.3V` devices,
- the `ECHO` line must therefore be connected through a voltage divider or logic-level shifter before reaching the ESP32.

Expected limitation:

- `HC-SR04` data are useful for demonstrating real sensing,
- but they are usually less suitable than a synthetic sinusoidal signal for clean FFT-based dominant-frequency estimation,
- therefore the virtual signal remains the primary source for required measurements and comparisons.

## Acceptance Checklist

- Maximum sampling frequency measured on the actual board
- FFT used to estimate dominant frequency
- Adaptive sampling frequency implemented
- Windowed average computed correctly
- Aggregate sent to edge server over MQTT/WiFi
- Aggregate sent to cloud over LoRaWAN/TTN
- Energy savings measured and discussed
- Per-window execution time measured
- Network data volume measured
- End-to-end latency measured
- LLM prompts and reflection documented
- Bonus section completed if time allows
- Optional: `HC-SR04` demo mode shown as a physical sensing extension
