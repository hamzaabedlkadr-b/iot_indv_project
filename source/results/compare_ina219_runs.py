import argparse
import json
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(description="Compare baseline and adaptive INA219 summaries.")
    parser.add_argument("--baseline", required=True, help="Path to baseline JSON summary.")
    parser.add_argument("--adaptive", required=True, help="Path to adaptive JSON summary.")
    parser.add_argument("--output-md", help="Optional output Markdown path.")
    return parser.parse_args()


def load_summary(path_str):
    return json.loads(Path(path_str).read_text(encoding="utf-8"))


def percentage_delta(baseline, adaptive):
    if baseline == 0:
        return 0.0
    return ((adaptive - baseline) / baseline) * 100.0


def build_markdown(baseline, adaptive):
    rows = [
        ("Duration (s)", baseline["duration_s"], adaptive["duration_s"]),
        ("Average current (mA)", baseline["avg_current_mA"], adaptive["avg_current_mA"]),
        ("Average power (mW)", baseline["avg_power_mW"], adaptive["avg_power_mW"]),
        ("Integrated energy (mWh)", baseline["energy_mWh"], adaptive["energy_mWh"]),
        ("Peak current (mA)", baseline["max_current_mA"], adaptive["max_current_mA"]),
        ("Peak power (mW)", baseline["max_power_mW"], adaptive["max_power_mW"]),
    ]

    lines = [
        "# INA219 Baseline vs Adaptive Comparison",
        "",
        "| Metric | Baseline | Adaptive | Delta vs baseline |",
        "| --- | --- | --- | --- |",
    ]

    for label, baseline_value, adaptive_value in rows:
        delta = percentage_delta(baseline_value, adaptive_value)
        lines.append(
            f"| {label} | `{baseline_value:.6f}` | `{adaptive_value:.6f}` | `{delta:.2f}%` |"
        )

    lines.extend(
        [
            "",
            "Interpretation:",
            f"- Negative deltas for average current, average power, or integrated energy mean the adaptive run consumed less than the fixed baseline.",
            "- Network-volume comparison should still be taken from the MQTT listener summaries because the INA219 measures electrical behavior, not payload bytes.",
        ]
    )

    return "\n".join(lines) + "\n"


def main():
    args = parse_args()
    baseline = load_summary(args.baseline)
    adaptive = load_summary(args.adaptive)
    markdown = build_markdown(baseline, adaptive)

    if args.output_md:
        output_path = Path(args.output_md)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(markdown, encoding="utf-8")

    print(markdown, end="")


if __name__ == "__main__":
    main()
