# MQTT Evidence Note

This note groups the `2026-04-18` home-network MQTT artifacts that are best suited for the final README and workshop presentation.

## What This Run Proves

- The Heltec board was connected to Wi-Fi and publishing real aggregate windows over MQTT.
- The local edge listener received consecutive messages without gaps.
- The clean reference profile remained stable at `40 Hz` sampling and `5 Hz` dominant frequency.
- The latency and payload-size numbers were captured from the same live run.

## Core Artifacts

- Structured capture: `source/results/summaries/mqtt_run_2026-04-18_listener.csv`
- Raw JSONL capture: `source/results/summaries/mqtt_run_2026-04-18_listener.jsonl`
- Report-ready summary: `source/results/summaries/mqtt_summary_2026-04-18_listener.md`
- Machine-readable summary: `source/results/summaries/mqtt_summary_2026-04-18_listener.json`
- Human-readable receive log: `source/results/summaries/mqtt_listener_2026-04-18_live.txt`

## Numbers To Cite

- Messages received: `5`
- Missing windows between first and last: `0`
- Sampling frequency average: `40.000 Hz`
- Dominant frequency average: `5.000 Hz`
- Listener latency average: `1234421.600 us`
- End-to-end latency average: `1587868.600 us`
- Average payload size: `454 bytes`

## Best Screenshots To Add Later

- Live listener proof:
  Open `source/results/summaries/mqtt_listener_2026-04-18_live.txt`
  Suggested caption: `Python edge listener receiving consecutive MQTT aggregate messages from the Heltec board on the home Wi-Fi network.`

- Summary proof:
  Open `source/results/summaries/mqtt_summary_2026-04-18_listener.md`
  Suggested caption: `Summary of the live MQTT timing run, including consecutive windows, payload size, listener latency, and end-to-end latency.`

## Suggested README Placement

- Put the live listener screenshot in the `MQTT / edge server validation` section.
- Put the summary screenshot or summary table in the `Latency and communication cost` section.
