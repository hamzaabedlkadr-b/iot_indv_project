# Signal Input

This module generates the project input samples.

Current modes:

- `virtual_signal` is the main path used for the assignment pipeline
- `sampling_benchmark` is used to measure the highest stable local sampling rate
- `hc_sr04_mode` remains an optional extension hook

Current responsibilities:

- produce timestamped samples at the requested sampling rate
- generate the repeatable synthetic signal used by the FFT and adaptive-sampling tests
- push samples into the shared queue with clear logging for validation

Configured signal profiles in `project_config.h`:

- `PROJECT_SIGNAL_PROFILE_CLEAN_REFERENCE`: the original clean dual-tone signal
- `PROJECT_SIGNAL_PROFILE_NOISY_REFERENCE`: the same signal plus low Gaussian-like noise
- `PROJECT_SIGNAL_PROFILE_ANOMALY_STRESS`: the noisy signal plus sparse large spikes

The module also tags each generated sample with the active signal profile and whether a synthetic anomaly was injected, so later stages can count anomalies per window.
