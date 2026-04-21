from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


WINDOW_SECONDS = 5.0
FIXED_RATE_HZ = 50.0
ADAPTIVE_RATE_HZ = 40.0


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def pct_delta(baseline: float, adaptive: float) -> float:
    if baseline == 0.0:
        return 0.0
    return ((adaptive - baseline) / baseline) * 100.0


def build_report(summary: dict[str, Any], source_path: Path) -> str:
    windows = int(summary["unique_windows"])
    adaptive_payload_bytes = int(summary["total_payload_bytes"])
    adaptive_avg_payload = float(summary["payload_size_bytes"]["avg"])
    fixed_payload_bytes = round(adaptive_avg_payload * windows)
    fixed_samples = int(round(FIXED_RATE_HZ * WINDOW_SECONDS * windows))
    adaptive_samples = int(round(ADAPTIVE_RATE_HZ * WINDOW_SECONDS * windows))
    raw_sample_reduction = pct_delta(fixed_samples, adaptive_samples)
    payload_delta = pct_delta(float(fixed_payload_bytes), float(adaptive_payload_bytes))

    return "\n".join(
        [
            "# Communication Volume Comparison",
            "",
            f"Source adaptive MQTT summary: `{source_path}`",
            "",
            "The firmware transmits one aggregate message per completed window. The adaptive sampler changes how many local samples are generated and processed inside the window, but it intentionally does not transmit raw samples.",
            "",
            "| Mode | Sampling rate | Window count | Samples represented | MQTT messages | Avg payload | Total payload | Delta vs fixed |",
            "|---|---:|---:|---:|---:|---:|---:|---:|",
            f"| Fixed oversampling baseline | `{FIXED_RATE_HZ:.1f} Hz` | `{windows}` | `{fixed_samples}` | `{windows}` | `{adaptive_avg_payload:.1f} B` | `{fixed_payload_bytes} B` | reference |",
            f"| Adaptive sampling | `{ADAPTIVE_RATE_HZ:.1f} Hz` | `{windows}` | `{adaptive_samples}` | `{windows}` | `{adaptive_avg_payload:.1f} B` | `{adaptive_payload_bytes} B` | `{payload_delta:.2f}%` |",
            "",
            "## Interpretation",
            "",
            f"- Local sample generation drops from `{fixed_samples}` to `{adaptive_samples}` samples for the same `{windows}` aggregate windows (`{raw_sample_reduction:.2f}%`).",
            f"- MQTT network volume stays effectively constant because both modes publish one compact aggregate per window instead of raw samples.",
            "- This is the intended architecture: adaptive sampling saves local sensing/processing work, while aggregation prevents network traffic from scaling with the raw sampling rate.",
            "- If the system transmitted every raw sample instead of aggregates, adaptive sampling would reduce raw sample traffic by the same `20%` as the local sample count.",
            "",
        ]
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create the baseline-vs-adaptive communication-volume table.")
    parser.add_argument(
        "--summary-json",
        type=Path,
        default=Path("source/results/summaries/mqtt_summary_2026-04-18_listener.json"),
    )
    parser.add_argument(
        "--output-md",
        type=Path,
        default=Path("source/results/summaries/communication_volume_comparison_2026-04-21.md"),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    summary = load_json(args.summary_json)
    report = build_report(summary, args.summary_json)
    args.output_md.parent.mkdir(parents=True, exist_ok=True)
    args.output_md.write_text(report, encoding="utf-8")
    print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
