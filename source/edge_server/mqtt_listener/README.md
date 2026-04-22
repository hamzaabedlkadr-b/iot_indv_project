# MQTT Listener

This folder contains the local edge-side listener used for Phase 8 `MQTT/WiFi` validation.

## Files

- `listen_aggregates.py`: subscribes to the aggregate topic, prints message summaries, and can write `CSV` and `JSONL` logs.
- `requirements.txt`: Python dependency list for the listener.

## Setup

Install the dependency:

```powershell
python -m pip install -r requirements.txt
```

## Example Usage

Listen to the firmware topic and write structured logs:

```powershell
python listen_aggregates.py --host <BROKER_HOST> --port 1883 --topic project/adaptive-sampling-node/aggregate --csv logs\aggregates.csv --jsonl logs\aggregates.jsonl
```

If the broker requires credentials:

```powershell
python listen_aggregates.py --host <BROKER_HOST> --port 1883 --topic project/adaptive-sampling-node/aggregate --username myuser --password mypass
```

For secure MQTT over TLS:

```powershell
python listen_aggregates.py --host broker.emqx.io --port 8883 --tls --topic iot_indv_project/hamza/adaptive-sampling-node/secure --client-id hamza-secure-edge-listener --csv logs\secure_aggregates.csv --jsonl logs\secure_aggregates.jsonl --limit 3
```

Use `--ca-file` only when the broker requires a custom CA bundle. Do not use `--tls-insecure-skip-verify` for final evidence.

Replace `<BROKER_HOST>` with the IP address or DNS name of your local broker.

## Logged Timing Fields

The listener computes:

- `listener_latency_us`: time from `published_at_us` in the payload to reception by the listener
- `end_to_end_latency_us`: time from `window_end_us` on the ESP32 to reception by the listener

The structured logs also preserve:

- `signal_profile`: which of the three virtual input modes produced the window
- `anomaly_count`: how many synthetic anomaly spikes were injected in that window

These logs are meant to feed the later Phase 10 performance analysis.
