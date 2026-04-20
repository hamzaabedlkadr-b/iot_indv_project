import argparse
import json
import re
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(description="Analyze INA219 TSV/CSV monitor logs.")
    parser.add_argument("--input", required=True, help="Path to the raw INA219 log file.")
    parser.add_argument("--label", default="", help="Short run label, e.g. baseline or adaptive.")
    parser.add_argument("--output-json", help="Optional path for the JSON summary.")
    parser.add_argument("--output-md", help="Optional path for the Markdown summary.")
    return parser.parse_args()


def parse_samples(path: Path):
    samples = []
    splitter = re.compile(r"[\t, ]+")

    for raw_line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or line.startswith("elapsed_ms"):
            continue

        parts = [part for part in splitter.split(line) if part]
        if len(parts) != 6:
            continue

        try:
            sample = {
                "elapsed_ms": float(parts[0]),
                "bus_voltage_V": float(parts[1]),
                "shunt_voltage_mV": float(parts[2]),
                "load_voltage_V": float(parts[3]),
                "current_mA": float(parts[4]),
                "power_mW": float(parts[5]),
            }
        except ValueError:
            continue

        samples.append(sample)

    return samples


def average(values):
    return sum(values) / len(values) if values else 0.0


def integrate_energy_mwh(samples):
    if len(samples) < 2:
        return 0.0

    energy_mwh = 0.0
    for previous, current in zip(samples, samples[1:]):
        dt_hours = (current["elapsed_ms"] - previous["elapsed_ms"]) / 1000.0 / 3600.0
        avg_power_mw = (previous["power_mW"] + current["power_mW"]) / 2.0
        energy_mwh += avg_power_mw * dt_hours
    return energy_mwh


def build_summary(samples, label):
    if not samples:
        raise ValueError("No INA219 samples could be parsed from the input log.")

    duration_s = (samples[-1]["elapsed_ms"] - samples[0]["elapsed_ms"]) / 1000.0 if len(samples) > 1 else 0.0

    summary = {
        "label": label,
        "sample_count": len(samples),
        "duration_s": duration_s,
        "avg_bus_voltage_V": average([sample["bus_voltage_V"] for sample in samples]),
        "avg_load_voltage_V": average([sample["load_voltage_V"] for sample in samples]),
        "avg_current_mA": average([sample["current_mA"] for sample in samples]),
        "avg_power_mW": average([sample["power_mW"] for sample in samples]),
        "max_current_mA": max(sample["current_mA"] for sample in samples),
        "max_power_mW": max(sample["power_mW"] for sample in samples),
        "min_power_mW": min(sample["power_mW"] for sample in samples),
        "energy_mWh": integrate_energy_mwh(samples),
    }

    if duration_s > 0.0:
        summary["energy_mJ"] = summary["energy_mWh"] * 3600.0
    else:
        summary["energy_mJ"] = 0.0

    return summary


def to_markdown(summary):
    title = summary["label"] or "ina219"
    return f"""# INA219 Summary - {title}

| Metric | Value |
| --- | --- |
| Samples parsed | `{summary["sample_count"]}` |
| Duration | `{summary["duration_s"]:.3f} s` |
| Average bus voltage | `{summary["avg_bus_voltage_V"]:.4f} V` |
| Average load voltage | `{summary["avg_load_voltage_V"]:.4f} V` |
| Average current | `{summary["avg_current_mA"]:.4f} mA` |
| Average power | `{summary["avg_power_mW"]:.4f} mW` |
| Peak current | `{summary["max_current_mA"]:.4f} mA` |
| Peak power | `{summary["max_power_mW"]:.4f} mW` |
| Minimum power | `{summary["min_power_mW"]:.4f} mW` |
| Integrated energy | `{summary["energy_mWh"]:.6f} mWh` |
| Integrated energy | `{summary["energy_mJ"]:.4f} mJ` |
"""


def main():
    args = parse_args()
    input_path = Path(args.input)
    samples = parse_samples(input_path)
    summary = build_summary(samples, args.label)

    if args.output_json:
        output_path = Path(args.output_json)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    if args.output_md:
        output_path = Path(args.output_md)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(to_markdown(summary), encoding="utf-8")

    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
