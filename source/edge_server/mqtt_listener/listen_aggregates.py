from __future__ import annotations

import argparse
import csv
import json
import ssl
import sys
import time
from pathlib import Path
from typing import Any

try:
    import paho.mqtt.client as mqtt
except ModuleNotFoundError as exc:  # pragma: no cover - import guard for local setup
    print(
        "Missing dependency: paho-mqtt. Install it with "
        "`python -m pip install -r requirements.txt`.",
        file=sys.stderr,
    )
    raise SystemExit(2) from exc


CSV_FIELDS = [
    "received_at_us",
    "topic",
    "device",
    "window_id",
    "window_start_us",
    "window_end_us",
    "published_at_us",
    "window_start_unix_us",
    "window_end_unix_us",
    "published_at_unix_us",
    "time_sync_ready",
    "signal_profile",
    "anomaly_count",
    "edge_delay_us",
    "sample_count",
    "sampling_frequency_hz",
    "dominant_frequency_hz",
    "average_value",
    "listener_latency_us",
    "end_to_end_latency_us",
    "raw_payload",
]


def now_us() -> int:
    return time.time_ns() // 1_000


def as_int(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return int(value)
    return None


def write_csv_record(path: Path, record: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    file_exists = path.exists()

    with path.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=CSV_FIELDS)
        if not file_exists:
            writer.writeheader()
        writer.writerow({field: record.get(field, "") for field in CSV_FIELDS})


def write_jsonl_record(path: Path, record: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as handle:
        handle.write(json.dumps(record, sort_keys=True))
        handle.write("\n")


def build_record(topic: str, payload_text: str) -> dict[str, Any]:
    received_at = now_us()
    record: dict[str, Any] = {
        "received_at_us": received_at,
        "topic": topic,
        "raw_payload": payload_text,
    }

    try:
        payload = json.loads(payload_text)
    except json.JSONDecodeError:
        record["listener_latency_us"] = None
        record["end_to_end_latency_us"] = None
        return record

    for field in (
        "device",
        "window_id",
        "window_start_us",
        "window_end_us",
        "published_at_us",
        "window_start_unix_us",
        "window_end_unix_us",
        "published_at_unix_us",
        "time_sync_ready",
        "signal_profile",
        "anomaly_count",
        "edge_delay_us",
        "sample_count",
        "sampling_frequency_hz",
        "dominant_frequency_hz",
        "average_value",
    ):
        record[field] = payload.get(field)

    published_at_unix_us = as_int(payload.get("published_at_unix_us"))
    window_end_unix_us = as_int(payload.get("window_end_unix_us"))

    if published_at_unix_us is not None and published_at_unix_us > 0 and received_at >= published_at_unix_us:
        record["listener_latency_us"] = received_at - published_at_unix_us
    else:
        record["listener_latency_us"] = None

    if window_end_unix_us is not None and window_end_unix_us > 0 and received_at >= window_end_unix_us:
        record["end_to_end_latency_us"] = received_at - window_end_unix_us
    else:
        record["end_to_end_latency_us"] = None

    return record


def print_record_summary(record: dict[str, Any]) -> None:
    summary = (
        "message"
        f" window={record.get('window_id', '-')}"
        f" profile={record.get('signal_profile', '-')}"
        f" fs={record.get('sampling_frequency_hz', '-')}"
        f"Hz dominant={record.get('dominant_frequency_hz', '-')}"
        f"Hz avg={record.get('average_value', '-')}"
        f" anomalies={record.get('anomaly_count', '-')}"
        f" listener_latency_us={record.get('listener_latency_us', '-')}"
        f" end_to_end_latency_us={record.get('end_to_end_latency_us', '-')}"
    )
    print(summary, flush=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Subscribe to the ESP32 aggregate topic and log incoming MQTT messages."
    )
    parser.add_argument("--host", default="127.0.0.1", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument(
        "--topic",
        default="project/adaptive-sampling-node/aggregate",
        help="MQTT topic to subscribe to",
    )
    parser.add_argument("--client-id", default="edge-listener", help="MQTT client id")
    parser.add_argument("--username", default="", help="Optional MQTT username")
    parser.add_argument("--password", default="", help="Optional MQTT password")
    parser.add_argument("--tls", action="store_true", help="Enable MQTT over TLS")
    parser.add_argument(
        "--ca-file",
        type=Path,
        default=None,
        help="Optional CA bundle path for TLS verification; defaults to system trust store",
    )
    parser.add_argument(
        "--tls-insecure-skip-verify",
        action="store_true",
        help="Disable TLS certificate verification; only use for debugging, not final evidence",
    )
    parser.add_argument("--keepalive", type=int, default=30, help="MQTT keepalive in seconds")
    parser.add_argument(
        "--csv",
        type=Path,
        default=None,
        help="Optional CSV output path for structured logs",
    )
    parser.add_argument(
        "--jsonl",
        type=Path,
        default=None,
        help="Optional JSONL output path for raw message records",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=0,
        help="Stop after this many messages; 0 keeps listening",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    received_messages = 0

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=args.client_id)
    if args.username:
        client.username_pw_set(args.username, args.password)
    if args.tls:
        context = ssl.create_default_context(cafile=str(args.ca_file) if args.ca_file else None)
        if args.tls_insecure_skip_verify:
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
        client.tls_set_context(context)

    def on_connect(
        inner_client: mqtt.Client,
        userdata: Any,
        connect_flags: dict[str, Any],
        reason_code: mqtt.ReasonCode,
        properties: Any,
    ) -> None:
        del userdata, connect_flags, properties
        if reason_code.is_failure:
            print(f"connection failed: {reason_code}", file=sys.stderr, flush=True)
            return

        inner_client.subscribe(args.topic, qos=1)
        print(
            f"subscribed to {args.topic} on {args.host}:{args.port} as {args.client_id}"
            f" tls={'enabled' if args.tls else 'disabled'}",
            flush=True,
        )

    def on_message(inner_client: mqtt.Client, userdata: Any, message: mqtt.MQTTMessage) -> None:
        nonlocal received_messages
        del userdata

        payload_text = message.payload.decode("utf-8", errors="replace")
        record = build_record(message.topic, payload_text)
        print_record_summary(record)

        if args.csv is not None:
            write_csv_record(args.csv, record)
        if args.jsonl is not None:
            write_jsonl_record(args.jsonl, record)

        received_messages += 1
        if args.limit > 0 and received_messages >= args.limit:
            inner_client.disconnect()

    client.on_connect = on_connect
    client.on_message = on_message

    print(
        f"connecting to broker {args.host}:{args.port} and waiting for topic {args.topic}"
        f" tls={'enabled' if args.tls else 'disabled'}"
        f" verify={'disabled' if args.tls_insecure_skip_verify else 'required'}",
        flush=True,
    )
    client.connect(args.host, args.port, args.keepalive)
    client.loop_forever()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
