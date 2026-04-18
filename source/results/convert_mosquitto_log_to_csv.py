from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Any


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


def as_int(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return int(value)
    return None


def parse_line(line: str) -> dict[str, Any] | None:
    text = line.strip()
    if not text:
        return None

    separator = "\t"
    first_tab = text.find(separator)
    if first_tab < 0:
        # PowerShell users sometimes save a literal `t instead of a real tab.
        separator = "`t"
        first_tab = text.find(separator)
        if first_tab < 0:
            return None

    topic_start = first_tab + len(separator)
    topic_sep = text.find(" ", topic_start)
    if topic_sep < 0:
        return None

    received_at_ms_text = text[:first_tab]
    topic = text[topic_start:topic_sep]
    payload_text = text[topic_sep + 1:]

    try:
        received_at_us = int(received_at_ms_text) * 1000
    except ValueError:
        return None

    record: dict[str, Any] = {
        "received_at_us": received_at_us,
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

    if published_at_unix_us is not None and published_at_unix_us > 0 and received_at_us >= published_at_unix_us:
        record["listener_latency_us"] = received_at_us - published_at_unix_us
    else:
        record["listener_latency_us"] = None

    if window_end_unix_us is not None and window_end_unix_us > 0 and received_at_us >= window_end_unix_us:
        record["end_to_end_latency_us"] = received_at_us - window_end_unix_us
    else:
        record["end_to_end_latency_us"] = None

    return record


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert timestamped mosquitto_sub logs into the project CSV format."
    )
    parser.add_argument("--input", type=Path, required=True, help="Raw mosquitto_sub log path")
    parser.add_argument("--output", type=Path, required=True, help="CSV output path")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)

    with args.input.open(encoding="utf-8") as input_handle, args.output.open(
        "w", newline="", encoding="utf-8"
    ) as output_handle:
        writer = csv.DictWriter(output_handle, fieldnames=CSV_FIELDS)
        writer.writeheader()

        for line in input_handle:
            record = parse_line(line)
            if record is None:
                continue
            writer.writerow({field: record.get(field, "") for field in CSV_FIELDS})

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
