# Metrics

This module tracks the runtime counters used during debugging and performance analysis.

It currently reports:

- sample, FFT, aggregate, and publish timestamps
- aggregate counters and latest edge latency
- communication counters for `MQTT` and prepared `LoRaWAN` payloads
- periodic heartbeat logs that make live runs easier to explain during evaluation
