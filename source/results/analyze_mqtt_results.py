from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
from statistics import mean
from typing import Any


def parse_int(value: str) -> int | None:
    text = value.strip()
    if text == "":
        return None
    try:
        return int(text)
    except ValueError:
        return None


def parse_float(value: str) -> float | None:
    text = value.strip()
    if text == "":
        return None
    try:
        return float(text)
    except ValueError:
        return None


def percentile(values: list[float], ratio: float) -> float | None:
    if not values:
        return None
    ordered = sorted(values)
    index = int(math.ceil((len(ordered) * ratio) - 1))
    index = max(0, min(index, len(ordered) - 1))
    return ordered[index]


def summarize_numeric(values: list[float]) -> dict[str, float | None]:
    if not values:
        return {
            "count": 0,
            "min": None,
            "max": None,
            "avg": None,
            "p95": None,
        }

    return {
        "count": len(values),
        "min": min(values),
        "max": max(values),
        "avg": mean(values),
        "p95": percentile(values, 0.95),
    }


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def summarize_rows(rows: list[dict[str, str]]) -> dict[str, Any]:
    received_times = [value for row in rows if (value := parse_int(row.get("received_at_us", ""))) is not None]
    window_ids = [value for row in rows if (value := parse_int(row.get("window_id", ""))) is not None]
    listener_latencies = [
        float(value)
        for row in rows
        if (value := parse_int(row.get("listener_latency_us", ""))) is not None
    ]
    end_to_end_latencies = [
        float(value)
        for row in rows
        if (value := parse_int(row.get("end_to_end_latency_us", ""))) is not None
    ]
    edge_delays = [
        float(value)
        for row in rows
        if (value := parse_int(row.get("edge_delay_us", ""))) is not None
    ]
    anomaly_counts = [
        float(value)
        for row in rows
        if (value := parse_int(row.get("anomaly_count", ""))) is not None
    ]
    sample_counts = [
        float(value)
        for row in rows
        if (value := parse_int(row.get("sample_count", ""))) is not None
    ]
    sampling_frequencies = [
        value
        for row in rows
        if (value := parse_float(row.get("sampling_frequency_hz", ""))) is not None
    ]
    dominant_frequencies = [
        value
        for row in rows
        if (value := parse_float(row.get("dominant_frequency_hz", ""))) is not None
    ]
    signal_profiles = sorted(
        {
            value
            for row in rows
            if (value := row.get("signal_profile", "").strip()) != ""
        }
    )
    payload_sizes = [len(row.get("raw_payload", "").encode("utf-8")) for row in rows]

    unique_windows = sorted(set(window_ids))
    missing_windows = 0
    if unique_windows:
        expected_windows = set(range(unique_windows[0], unique_windows[-1] + 1))
        missing_windows = len(expected_windows.difference(unique_windows))

    received_span_s = None
    if len(received_times) >= 2:
        received_span_s = (max(received_times) - min(received_times)) / 1_000_000.0

    return {
        "messages_received": len(rows),
        "unique_windows": len(unique_windows),
        "missing_windows_between_first_and_last": missing_windows,
        "first_window_id": min(unique_windows) if unique_windows else None,
        "last_window_id": max(unique_windows) if unique_windows else None,
        "received_span_s": received_span_s,
        "signal_profiles_seen": signal_profiles,
        "listener_latency_us": summarize_numeric(listener_latencies),
        "end_to_end_latency_us": summarize_numeric(end_to_end_latencies),
        "edge_delay_us": summarize_numeric(edge_delays),
        "anomaly_count": summarize_numeric(anomaly_counts),
        "sample_count": summarize_numeric(sample_counts),
        "sampling_frequency_hz": summarize_numeric(sampling_frequencies),
        "dominant_frequency_hz": summarize_numeric(dominant_frequencies),
        "payload_size_bytes": summarize_numeric([float(value) for value in payload_sizes]),
        "total_payload_bytes": sum(payload_sizes),
    }


def format_value(value: Any) -> str:
    if value is None:
        return "-"
    if isinstance(value, float):
        return f"{value:.3f}"
    return str(value)


def build_markdown_report(summary: dict[str, Any], input_path: Path) -> str:
    lines = [
        "# MQTT Result Summary",
        "",
        f"Input file: `{input_path}`",
        "",
        "## Overview",
        "",
        f"- Messages received: {format_value(summary['messages_received'])}",
        f"- Unique windows: {format_value(summary['unique_windows'])}",
        f"- Missing windows between first and last: {format_value(summary['missing_windows_between_first_and_last'])}",
        f"- First window id: {format_value(summary['first_window_id'])}",
        f"- Last window id: {format_value(summary['last_window_id'])}",
        f"- Received span (s): {format_value(summary['received_span_s'])}",
        f"- Signal profiles seen: {', '.join(summary['signal_profiles_seen']) if summary['signal_profiles_seen'] else '-'}",
        f"- Total payload bytes: {format_value(summary['total_payload_bytes'])}",
        "",
        "## Metrics",
        "",
    ]

    for key in (
        "listener_latency_us",
        "end_to_end_latency_us",
        "edge_delay_us",
        "anomaly_count",
        "sample_count",
        "sampling_frequency_hz",
        "dominant_frequency_hz",
        "payload_size_bytes",
    ):
        metric = summary[key]
        lines.extend(
            [
                f"### {key}",
                "",
                f"- Count: {format_value(metric['count'])}",
                f"- Min: {format_value(metric['min'])}",
                f"- Avg: {format_value(metric['avg'])}",
                f"- P95: {format_value(metric['p95'])}",
                f"- Max: {format_value(metric['max'])}",
                "",
            ]
        )

    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarize CSV logs produced by the MQTT edge listener."
    )
    parser.add_argument("--input", type=Path, required=True, help="Input CSV path")
    parser.add_argument("--output-json", type=Path, default=None, help="Optional JSON summary path")
    parser.add_argument("--output-md", type=Path, default=None, help="Optional Markdown summary path")
    return parser.parse_args()


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def main() -> int:
    args = parse_args()
    rows = load_rows(args.input)
    summary = summarize_rows(rows)
    markdown = build_markdown_report(summary, args.input)

    print(markdown)

    if args.output_json is not None:
        args.output_json.parent.mkdir(parents=True, exist_ok=True)
        args.output_json.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    if args.output_md is not None:
        write_text(args.output_md, markdown)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
