# Sampling Control

This module consumes FFT results and decides whether the sampling rate should change.

It currently:

- reads the dominant frequency from each completed window
- applies the project's adaptive-sampling rule with minimum and maximum bounds
- updates the shared sampling target used by the signal-input task
- logs both rate changes and stable "hold" decisions for later explanation

When `PROJECT_ENABLE_ADAPTIVE_SAMPLING` is set to `0U` in `project_config.h`, the same module switches into a fixed baseline mode and keeps the initial sampling frequency unchanged. This is used for the planned baseline-versus-adaptive energy measurements.
