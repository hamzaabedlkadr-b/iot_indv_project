# Signal Processing

This module owns the per-window analysis pipeline.

It currently:

- buffers incoming samples into fixed analysis windows
- runs the spectral analysis used to estimate the dominant frequency
- computes the required per-window aggregate values
- fans one completed aggregate out to both the `MQTT` and `LoRaWAN` paths
