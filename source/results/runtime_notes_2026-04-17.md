# Runtime Notes - 2026-04-17

This note is a historical snapshot from the pre-`TTN` validation stage. It is still useful for the early live `MQTT` bring-up story, but it does not describe the later integrated `LoRaWAN` proof from `2026-04-20`. For the current cloud-side status, use [`lorawan_evidence_2026-04-20.md`](./lorawan_evidence_2026-04-20.md) and [`../docs/CURRENT_PROGRESS_REPORT.md`](../docs/CURRENT_PROGRESS_REPORT.md).

## Session Summary

- Board detected as `COM3`
- Firmware build succeeded with local PlatformIO
- Firmware upload to the Heltec succeeded
- Serial boot logs were captured from the live board
- firmware was later reconfigured for the current home WiFi and reflashed successfully

## Confirmed Working On Real Hardware

- the firmware boots correctly on the Heltec
- the virtual sensor starts in `virtual_sensor` mode
- the signal-processing pipeline fills windows and computes aggregates
- FFT detection identifies `5.00 Hz` as the dominant component
- adaptive sampling updates from `50.0 Hz` to `40.0 Hz`
- the `MQTT` path prepares real aggregate payloads
- the `LoRaWAN` path prepares compact aggregate payloads in stub-ready mode

## Key Observations From The Serial Logs

- `app_main` reported communication mode `both`
- the first completed window used `250` samples at `50.0 Hz`
- later windows used `200` samples at `40.0 Hz` after adaptation
- `comm_lorawan` prepared payloads such as:
  `000100C8019001F4FFE8`
- metrics heartbeats confirmed aggregates were being produced continuously

## Current Blocker

The firmware is still configured for:

- a temporary lab WiFi SSID
- a temporary broker host on a different private subnet

During this live run:

- the laptop was on a different private network
- the configured broker host was not reachable from the current environment
- `comm_mqtt` kept retrying and no MQTT messages were actually sent

## Meaning

This session validates the board-side firmware logic, but not the final Phase 8 success condition yet.

What is already proven:

- firmware build and flash pipeline
- real serial boot on hardware
- real signal generation, FFT, adaptive control, and aggregate generation
- real preparation of MQTT and LoRaWAN payloads

What still needs a matching network setup:

- successful WiFi connection to the intended SSID
- successful MQTT broker connection
- edge listener receiving real messages from the board

## Next Recommended Step

Before the next live MQTT run, align the runtime environment in one of these ways:

1. connect the laptop and board to the same WiFi network and run a reachable broker there
2. change `project_config.h` so the board uses the current WiFi network and a reachable broker IP

After that, rerun the local MQTT checklist and save the resulting `CSV`, `JSONL`, and summary report.

## Follow-Up Session On The Home Network

The firmware was updated to use:

- the local WiFi network
- the local broker host on the same private subnet

The second live run confirmed:

- the Heltec now connects to the home WiFi network
- the board receives an IP on the same private subnet as the broker
- the MQTT client now starts only after WiFi gets an IP address

The remaining issue after this improvement:

- the board still does not complete the MQTT connection
- local broker tools on the laptop receive no published messages
- the most likely blocker is Windows inbound filtering on port `1883`

Meaning:

- WiFi configuration is now correct
- firmware-side MQTT startup timing is improved
- the next step is allowing inbound MQTT traffic to the broker on this laptop

## Final MQTT Validation On The Home Network

After the Windows network profile was changed to `Private` and the inbound firewall rule for port `1883` was added:

- the Heltec connected to the configured WiFi network
- the board obtained a valid private IP address
- the MQTT client reported `MQTT broker connected`
- the firmware published real aggregate messages
- the local Mosquitto subscriber received the topic payload successfully

Example received payload:

```text
project/adaptive-sampling-node/aggregate {"device":"adaptive-sampling-node","window_id":80,"window_start_us":401081330,"window_end_us":406056393,"published_at_us":406456503,"edge_delay_us":400110,"sample_count":200,"sampling_frequency_hz":40.0,"dominant_frequency_hz":5.00,"average_value":-0.0001}
```

This confirms that Phase 8 is now validated end-to-end on real hardware for the home WiFi setup.

## SNTP-Based Latency Validation And Payload Fix

To make the latency measurements comparable across the ESP32 and the laptop:

- the MQTT payload was extended with Unix timestamps derived from `SNTP`
- the edge listener now prefers `published_at_unix_us` and `window_end_unix_us`
- the firmware waits for WiFi, starts `SNTP`, and reports `time_sync_ready=true` in the payload

During the first timing run after this change, the new fields exposed a useful bug:

- the MQTT payload buffer was too small once the Unix timestamp fields were added
- the broker still received messages, but the JSON was truncated before the final fields
- the shared transmission payload buffer was increased from `320` to `512` bytes
- the MQTT builder now logs an explicit warning if a payload would be truncated again

The corrected live measurement run produced clean listener artifacts in:

- `source/results/summaries/mqtt_run_2026-04-17_listener_clean.csv`
- `source/results/summaries/mqtt_run_2026-04-17_listener_clean.jsonl`
- `source/results/summaries/mqtt_summary_2026-04-17_listener_clean.json`
- `source/results/summaries/mqtt_summary_2026-04-17_listener_clean.md`

Measured from windows `9` to `13` on the home MQTT setup:

- average `listener_latency_us`: `856088.4`
- average `end_to_end_latency_us`: `1222508.4`
- average `edge_delay_us`: `366420.0`
- payload size: about `393` to `394` bytes per aggregate
- dominant frequency remained stable at `5.00 Hz`
- adaptive sampling remained stable at `40.0 Hz`

This is the first clean Phase 10 latency dataset captured from the real board with synchronized wall-clock timestamps.

## Three-Signal Firmware Extension

The firmware was extended further so the virtual sensor now supports three selectable profiles:

- `clean_reference`
- `noisy_reference`
- `anomaly_stress`

The selected profile is now carried through:

- the live firmware logs,
- the aggregate structure,
- the MQTT payload,
- the Python listener `CSV` and `JSONL`,
- the summary analyzer output

The updated firmware was flashed to the Heltec and validated again on the home MQTT setup. A clean post-sync run was saved in:

- `source/results/summaries/mqtt_run_2026-04-17_clean_profile_synced.csv`
- `source/results/summaries/mqtt_run_2026-04-17_clean_profile_synced.jsonl`
- `source/results/summaries/mqtt_summary_2026-04-17_clean_profile_synced.json`
- `source/results/summaries/mqtt_summary_2026-04-17_clean_profile_synced.md`

That run confirmed:

- `signal_profile=clean_reference` appears in the live MQTT payload
- `anomaly_count=0` is reported correctly for the clean baseline
- the analyzer now reports which signal profiles were seen in a dataset

Additional live runs were also captured for the other two profiles:

- `source/results/summaries/mqtt_run_2026-04-17_noisy_profile_synced.csv`
- `source/results/summaries/mqtt_summary_2026-04-17_noisy_profile_synced.md`
- `source/results/summaries/mqtt_run_2026-04-17_anomaly_profile_synced.csv`
- `source/results/summaries/mqtt_summary_2026-04-17_anomaly_profile_synced.md`

Observed from those real-board runs:

- `noisy_reference` preserved `dominant_frequency_hz=5.00` across the saved windows
- `anomaly_stress` also preserved `dominant_frequency_hz=5.00` in the saved windows
- the anomaly profile produced non-zero per-window `anomaly_count` values with an average of `2.8` spikes per saved window in the sample run

At the end of the session, the firmware configuration and board were returned to the safer default `clean_reference` profile.
