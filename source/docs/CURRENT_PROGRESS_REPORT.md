# Current Progress Report

Date: `2026-04-18`

This report summarizes the project state up to the current checkpoint. It is meant to be the readable, up-to-date companion to the older historical report in [`PHASE1_REPORT.tex`](./PHASE1_REPORT.tex), which still documents Phases `1` to `7` in depth.

The goal of this document is to explain:

- what has already been implemented and validated
- what measurements exist today
- what evidence files support the current claims
- what is still pending before the final submission

## 1. Executive Summary

The project now has a working `ESP32 + FreeRTOS` pipeline that:

- generates the required signal in firmware through a virtual sensor
- collects samples into fixed-duration windows
- estimates the dominant signal frequency
- adapts the sampling frequency at runtime
- computes a per-window aggregate
- publishes real aggregate messages to a nearby edge server over `MQTT/WiFi`
- prepares compact aggregate payloads for `LoRaWAN + TTN`

The main local validation milestone is complete:

- the `MQTT/WiFi` edge path has been validated end-to-end on the real Heltec board
- synchronized timestamps have been added and used for real latency measurements
- three signal profiles have been validated on real hardware
- a `BetterSerialPlotter` view has been captured for live signal-processing visualization

The project is not fully finished yet. The remaining work is:

- a real `LoRaWAN/TTN` validation run on campus
- energy measurements using a cleaner `INA219` setup when two Heltec boards are available
- one live `MQTTS` validation run if a TLS-capable broker is available

## 2. Current Phase Status

| Phase | Topic | Status | Notes |
| --- | --- | --- | --- |
| `1` | Environment setup | Complete | Board, PlatformIO, serial, and broker environment were validated |
| `2` | Firmware skeleton | Complete | Modular FreeRTOS structure and shared data types are in place |
| `3` | Virtual sensor | Complete | Required sinusoidal signal is generated in firmware |
| `4` | Maximum sampling frequency | Complete | Strict benchmark performed on the real board |
| `5` | Frequency analysis | Complete | Dominant frequency detection implemented and validated |
| `6` | Adaptive sampling | Complete | Runtime update from `50 Hz` to `40 Hz` validated |
| `7` | Aggregate computation | Complete | Window averages are computed and passed downstream |
| `8` | MQTT over WiFi | Complete | End-to-end home-network validation completed on hardware |
| `9` | LoRaWAN + TTN | Partial | Payload path is implemented, but real TTN proof is still pending |
| `10` | Performance measurements | Partial | Latency and payload-size evidence exist; energy is still pending |
| `11` | Optional physical sensor / extras | Optional | Not required for the core project |
| `12` | Final packaging and write-up | In progress | README, evidence map, and screenshot assets are being prepared |

## 3. Project Architecture

The final implementation path is:

```text
virtual signal
    -> sampling task
    -> FFT / dominant-frequency detection
    -> adaptive sampling control
    -> 5 s aggregate window
    -> MQTT over WiFi to nearby edge listener
    -> compact LoRaWAN payload for TTN
```

The modular structure is important for the workshop because each task has a clear responsibility:

- `signal_input`: sample generation and runtime sampling-rate changes
- `signal_processing`: window buffering, dominant-frequency estimation, and aggregate creation
- `sampling_control`: adaptive policy
- `comm_mqtt`: edge publishing
- `comm_lorawan`: compact uplink payload preparation
- `metrics`: runtime counters and timing support

## 4. Hardware And Software Used

### Hardware

- `Heltec WiFi LoRa 32 V3` compatible board
- integrated `WiFi`
- integrated `868 MHz LoRa` radio
- integrated OLED
- laptop running the local broker and edge listener

### Software

- `PlatformIO`
- `ESP-IDF`
- `Mosquitto`
- Python-based `MQTT` listener and result analyzer
- `BetterSerialPlotter`

## 5. Signal Model And Profiles

The core virtual input signal is based on the assignment requirement:

```text
2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)
```

This produces two spectral components:

- `3 Hz`
- `5 Hz`

The current implementation uses three profiles:

| Profile | Purpose | Notes |
| --- | --- | --- |
| `clean_reference` | baseline | deterministic signal without noise |
| `noisy_reference` | robustness | same signal plus low random noise |
| `anomaly_stress` | bonus / stress | noisy signal plus sparse spike injection |

The selected profile is carried through the aggregate object and the MQTT payload as `signal_profile`, and anomaly windows also report `anomaly_count`.

## 6. Maximum Sampling Frequency Benchmark

The strict benchmark from Phase `4` tested multiple target rates and classified a rate as stable only if timing stayed clean under the project rule.

### Measured Result Table

| Target | Achieved | Drops | Misses | Stable | Observation |
| --- | --- | --- | --- | --- | --- |
| `50 Hz` | `50.25 Hz` | `0` | `0` | Yes | clean stable run |
| `100 Hz` | `100.25 Hz` | `0` | `1` | No | very close, but one miss occurred |
| `200 Hz` | `200.25 Hz` | `0` | `2` | No | throughput held, timing did not |
| `250 Hz` | `250.25 Hz` | `0` | `12` | No | watchdog instability started |
| `500 Hz` | `500.25 Hz` | `0` | `26` | No | repeated instability |
| `1000 Hz` | `1000.00 Hz` | `1` | `55` | No | severe instability |

### Interpretation

The important conclusion is:

- the board can technically generate and consume data above `50 Hz`
- but under the current strict stability rule, the highest clean operating point is `50 Hz`

This is why the project uses:

- `50 Hz` as the baseline oversampling reference
- `40 Hz` as the first stable adaptive rate for the current signal

## 7. FFT And Adaptive Sampling

After the first stable `50 Hz` window, the firmware detects the dominant component correctly and updates the rate:

```text
Window 0 ready | fs=50.0Hz samples=250 avg=0.0041 min=-5.6139 max=5.6139 duration=4.97s
Adaptive sampling update | window=0 dominant=5.00 Hz peak=2.0000 previous_fs=50.0 Hz new_fs=40.0 Hz
Applying adaptive sampling frequency 40.0 Hz
```

### Why `40 Hz` Is Correct

The current policy uses:

- oversampling factor: `8x`
- dominant frequency: `5 Hz`

So:

```text
5 Hz * 8 = 40 Hz
```

That result matches the validated runtime behavior.

## 8. Aggregate Computation

Each completed window becomes an aggregate object that contains:

- `window_id`
- `window_start_us`
- `window_end_us`
- `sample_count`
- `sampling_frequency_hz`
- `dominant_frequency_hz`
- `average_value`
- `signal_profile`
- `anomaly_count`

The first clean aggregate appeared at the initial baseline rate:

```text
Aggregate result | window=0 fs=50.0Hz samples=250 avg=0.0041 dominant=5.00 Hz
```

And after adaptation, the aggregate pipeline remained correct:

```text
Aggregate result | window=1 fs=40.0Hz samples=200 avg=-0.0000 dominant=5.00 Hz
```

This confirms that:

- the aggregate function is correct
- the adaptive transition does not break the aggregate path
- the edge and cloud paths can consume real summary data instead of placeholders

## 9. MQTT Over WiFi Validation

The local edge path was fully validated on the real board on the home network.

### What Was Proven

- the Heltec connected to the home WiFi
- the board reached the local broker
- the edge listener received real aggregate messages
- the messages contained synchronized timestamps
- the saved run had no missing windows

### Latest Clean Listener Run

Source:

- [`../results/summaries/mqtt_summary_2026-04-18_listener.md`](../results/summaries/mqtt_summary_2026-04-18_listener.md)

#### Overview

| Metric | Value |
| --- | --- |
| Messages received | `5` |
| Unique windows | `5` |
| Missing windows | `0` |
| First window | `296` |
| Last window | `300` |
| Received span | `20.002 s` |
| Signal profile | `clean_reference` |
| Total payload bytes | `2270` |

#### Timing Metrics

| Metric | Min | Avg | P95 | Max |
| --- | --- | --- | --- | --- |
| `listener_latency_us` | `1232577` | `1234421.6` | `1239997` | `1239997` |
| `end_to_end_latency_us` | `1584866` | `1587868.6` | `1596541` | `1596541` |
| `edge_delay_us` | `352289` | `353447.0` | `356544` | `356544` |

#### Signal Metrics

| Metric | Min | Avg | Max |
| --- | --- | --- | --- |
| `sample_count` | `200` | `200.0` | `200` |
| `sampling_frequency_hz` | `40.0` | `40.0` | `40.0` |
| `dominant_frequency_hz` | `5.0` | `5.0` | `5.0` |
| `payload_size_bytes` | `454` | `454.0` | `454` |

### Interpretation

The edge path is no longer just implemented in code. It has been validated end-to-end with:

- real board
- real WiFi
- real broker
- real listener output
- saved artifacts

## 10. Three-Signal Validation

The bonus-ready signal profiles were also validated through real-board MQTT runs.

### Comparison Table

| Profile | Avg dominant frequency | Avg sampling frequency | Avg anomaly count | Avg payload size | Avg listener latency |
| --- | --- | --- | --- | --- | --- |
| `clean_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `446.0 B` | `854014.8 us` |
| `noisy_reference` | `5.000 Hz` | `40.000 Hz` | `0.000` | `445.6 B` | `845563.8 us` |
| `anomaly_stress` | `5.000 Hz` | `40.000 Hz` | `2.800` | `444.6 B` | `823576.4 us` |

### Main Observations

- all three profiles preserved `5.00 Hz` as the dominant frequency in the saved windows
- the anomaly profile produced non-zero `anomaly_count` values as expected
- the payload size remained almost constant because each window still produces one compact aggregate

### Why This Matters

This is useful for the final workshop because it shows:

- the adaptive logic still behaves correctly under noise
- anomaly injection is real, not just described
- the project already covers the `3 signal` bonus direction

## 11. BetterSerialPlotter Validation

The project also includes a simplified serial stream for `BetterSerialPlotter`.

This visual evidence shows:

- current sampling frequency
- dominant frequency
- aggregate average

Current saved screenshot:

- [`../pics/2026-04-18_better_serial_plotter_live_view.png`](../pics/2026-04-18_better_serial_plotter_live_view.png)

This is presentation evidence for the live signal-processing pipeline, not for MQTT or TTN delivery.

## 12. LoRaWAN And TTN Status

The `LoRaWAN` path is implemented up to the payload-preparation stage.

What is already true:

- the aggregate is serialized into a compact uplink payload
- the LoRa path is integrated into the same pipeline as MQTT
- the runtime logs show prepared payloads such as:
  `000100C8019001F4FFE8`

What is still missing:

- a real uplink through a reachable gateway
- `TTN` live data screenshots
- decoded cloud-side proof

So the correct current description is:

- `LoRaWAN/TTN` path is `implemented and stub-ready`
- `live TTN validation is still pending`

## 13. Secure MQTT Status

The MQTT client now supports:

- plain `mqtt://`
- verified `mqtts://`
- optional username/password authentication
- certificate verification through the ESP-IDF CA bundle

However, the captured saved run is still the plain local-broker run.

So the honest current status is:

- secure MQTT support is implemented in firmware
- a live secure-broker validation run is still pending

## 14. Energy Measurement Status

Energy is a required grading item, but the cleanest measurement setup needs two boards:

- one board under test
- one board or monitor path reading the `INA219`

Because only one Heltec is currently available, the project keeps the energy section prepared but deferred.

What has already been done:

- the firmware now has a baseline-versus-adaptive switch
- the measurement runbook has been written
- the `INA219` setup for the Heltec V3 has been documented

What is still missing:

- real measured power or energy values
- screenshots of the measurement setup
- final baseline-versus-adaptive comparison table

## 15. Communication Volume Discussion

One subtle but important observation is that this project sends one aggregate per completed window, not raw samples over MQTT.

That means:

- adaptive sampling reduces local sampling and processing work
- but it does not automatically reduce the MQTT message size very much

This is exactly what the saved payload summaries show:

- payload sizes remain around `444` to `454` bytes depending on the profile

So the communication-volume section in the final report should explain that:

- the main gain from adaptive sampling in this implementation is local efficiency
- the network payload count per window is mostly unchanged

## 16. Build And Resource Status

The current firmware still builds successfully after the recent additions such as:

- three signal profiles
- secure MQTT support
- baseline/adaptive config switch

Current build snapshot:

| Resource | Usage |
| --- | --- |
| RAM | `38520 B` (`11.8%`) |
| Flash | `973469 B` (`92.8%`) |

This is still acceptable, but flash headroom is now limited.

## 17. Evidence Available Right Now

Useful current evidence files include:

- [`PHASE1_REPORT.tex`](./PHASE1_REPORT.tex)
- [`../results/runtime_notes_2026-04-17.md`](../results/runtime_notes_2026-04-17.md)
- [`../results/mqtt_evidence_2026-04-18.md`](../results/mqtt_evidence_2026-04-18.md)
- [`../results/screenshot_inventory_2026-04-18.md`](../results/screenshot_inventory_2026-04-18.md)
- [`../results/screenshots/`](../results/screenshots/)
- [`../results/summaries/`](../results/summaries/)

## 18. Remaining Work

The remaining work before the final submission is:

| Item | Status | Notes |
| --- | --- | --- |
| `LoRaWAN/TTN` live proof | pending | must be done on campus or under real gateway coverage |
| Energy measurements | pending | deferred until a cleaner two-board setup is available |
| Secure MQTT live TLS proof | pending | firmware support exists, but no saved live TLS run yet |
| Prompt log curation | pending | intentionally left for later |
| Final short report | pending | this detailed report will make that much easier |

## 19. Conclusion

At this checkpoint, the project has moved well beyond the early scaffolding phases.

The most important completed achievements are:

- stable virtual-signal generation on the real board
- measured maximum stable sampling benchmark
- correct dominant-frequency detection
- working adaptive-sampling transition from `50 Hz` to `40 Hz`
- correct per-window aggregate computation
- full home-network `MQTT/WiFi` validation on hardware
- real latency measurements based on synchronized timestamps
- three validated signal profiles including anomaly-aware evidence

The most important missing items are now concentrated and clear:

- real `TTN` uplink evidence
- energy comparison measurements
- one secure `MQTTS` validation run

This means the project is already in a strong state for the workshop: the core pipeline works, the edge path is proven, and the remaining work is focused on final measurement and cloud validation rather than on missing core functionality.
