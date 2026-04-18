# Assignment Brief

This file is a cleaned repository copy of the course assignment so the submission stays self-contained.

## Core Goal

Build an IoT system on an `ESP32` running `FreeRTOS` that:

- generates or captures a sensor-like input signal of the form `SUM(a_k * sin(f_k))`
- measures the maximum stable sampling frequency of the hardware
- uses an `FFT` to estimate dominant frequency content
- adapts the sampling frequency accordingly
- computes an aggregate over a time window
- sends the aggregate to a nearby edge server using `MQTT` over `WiFi`
- sends the aggregate to the cloud using `LoRaWAN + TTN`

## Performance Evaluation Requirements

The system should measure and discuss:

- energy savings of adaptive sampling versus the original over-sampled mode
- per-window execution time
- network data volume in adaptive versus over-sampled mode
- end-to-end latency from data generation to edge-server reception

## LLM Usage Requirement

The assignment explicitly requires documenting the use of an `LLM`.

The final repository should therefore include:

- a meaningful prompt history
- reflections on the quality of generated code
- a discussion of opportunities and limitations of the `LLM` workflow

## Bonus Work

The bonus section asks for:

- at least `3` different input signals
- a noisy/anomalous signal variant
- anomaly-aware filtering with `Z-score` and `Hampel`
- discussion of anomaly impact on `FFT`, adaptive sampling, latency, memory, and energy

## Submission Expectations

The `GitHub` repository should include:

- all code and scripts required to run the system
- setup and execution instructions
- technical explanations answering the assignment questions
- measurement evidence and discussion
- a clear main `README.md` for workshop presentation

## Evaluation Areas

The workshop evaluation focuses on:

- correctness of frequency estimation and adaptive sampling
- correctness of aggregate computation
- energy, communication-volume, and latency evaluation
- secure `MQTT` communication
- quality of the `FreeRTOS` code
- quality and clarity of the repository presentation
- bonus signal coverage
