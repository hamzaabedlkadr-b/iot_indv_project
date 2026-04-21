# WiFi MQTT Evidence - 2026-04-21

This note preserves the successful fresh WiFi/MQTT run after switching to the `sapienza` WiFi network.

## Serial Proof

Key lines from the live board run:

```text
Planned MQTT endpoint: 10.2.92.159:1883
WiFi station started; connecting to SSID 'sapienza'
WiFi connected | ip=10.2.67.246
MQTT broker connected
Published MQTT aggregate | window=0 msg_id=14690 topic=project/adaptive-sampling-node/aggregate
Published MQTT aggregate | window=1 msg_id=4205 topic=project/adaptive-sampling-node/aggregate
MQTT heartbeat | wifi=1 mqtt=1 pending=0 prepared=5 sent=5
```

These lines prove that the board:

- joined the new WiFi network
- reached the local MQTT broker
- published real aggregate messages

## Listener Artifacts

The fresh edge-listener run saved:

- [`summaries/mqtt_run_2026-04-21_fresh_wifi.csv`](./summaries/mqtt_run_2026-04-21_fresh_wifi.csv)
- [`summaries/mqtt_run_2026-04-21_fresh_wifi.jsonl`](./summaries/mqtt_run_2026-04-21_fresh_wifi.jsonl)
- [`summaries/mqtt_summary_2026-04-21_fresh_wifi.md`](./summaries/mqtt_summary_2026-04-21_fresh_wifi.md)
- [`summaries/mqtt_summary_2026-04-21_fresh_wifi.json`](./summaries/mqtt_summary_2026-04-21_fresh_wifi.json)

Summary:

| Metric | Value |
| --- | --- |
| Messages received | `6` |
| Unique windows | `6` |
| Missing windows | `0` |
| First window | `6` |
| Last window | `11` |
| Signal profile | `clean_reference` |
| Average sampling frequency | `40.000 Hz` |
| Average dominant frequency | `5.000 Hz` |
| Average sample count | `200` |
| Average payload size | `401.667 B` |
| Average edge delay | `346039.500 us` |

## Note About Latency

This fresh run has `time_sync_ready=false`, so listener and end-to-end latency fields are empty. The older synchronized MQTT run remains the best latency evidence:

- [`summaries/mqtt_summary_2026-04-18_listener.md`](./summaries/mqtt_summary_2026-04-18_listener.md)
