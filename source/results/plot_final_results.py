"""Generate static plots for the final measurement results.

The plots are derived from the saved summary artifacts in source/results/summaries.
They are intended for the README, report, and workshop walkthrough.
"""

from __future__ import annotations

import csv
import json
from pathlib import Path

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]
PICS_DIR = ROOT / "source" / "pics"
SUMMARY_DIR = ROOT / "source" / "results" / "summaries"
PLOT_DATE = "2026-04-22"


def read_json(name: str) -> dict:
    with (SUMMARY_DIR / name).open(encoding="utf-8") as handle:
        return json.load(handle)


def annotate_bars(ax, bars, fmt: str = "{:.1f}") -> None:
    for bar in bars:
        height = bar.get_height()
        ax.annotate(
            fmt.format(height),
            xy=(bar.get_x() + bar.get_width() / 2, height),
            xytext=(0, 4),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8,
        )


def set_common_style(ax, title: str, ylabel: str) -> None:
    ax.set_title(title, fontweight="bold")
    ax.set_ylabel(ylabel)
    ax.grid(True, axis="y", alpha=0.25)


def plot_energy() -> Path:
    baseline = read_json("ina219_baseline_2026-04-21.json")
    adaptive = read_json("ina219_adaptive_2026-04-21.json")
    deep_sleep = read_json("ina219_deepsleep_2026-04-21.json")
    runs = [
        ("Baseline\n50 Hz", baseline),
        ("Adaptive\n40 Hz", adaptive),
        ("Adaptive +\ndeep sleep", deep_sleep),
    ]

    labels = [label for label, _ in runs]
    avg_power = [run["avg_power_mW"] for _, run in runs]
    energy = [run["energy_mWh"] for _, run in runs]
    colors = ["#64748b", "#0f766e", "#2563eb"]

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.5), dpi=160)

    bars = axes[0].bar(labels, avg_power, color=colors)
    set_common_style(axes[0], "Average power", "mW")
    annotate_bars(axes[0], bars, "{:.1f}")

    bars = axes[1].bar(labels, energy, color=colors)
    set_common_style(axes[1], "Integrated energy over run", "mWh")
    annotate_bars(axes[1], bars, "{:.2f}")

    baseline_energy = baseline["energy_mWh"]
    adaptive_delta = (adaptive["energy_mWh"] - baseline_energy) / baseline_energy * 100.0
    sleep_delta = (deep_sleep["energy_mWh"] - baseline_energy) / baseline_energy * 100.0
    axes[1].text(
        0.02,
        0.92,
        f"adaptive: {adaptive_delta:+.2f}%\ndeep sleep: {sleep_delta:+.2f}%",
        transform=axes[1].transAxes,
        color="#334155",
        fontsize=9,
        bbox={"boxstyle": "round,pad=0.35", "facecolor": "#f8fafc", "edgecolor": "#cbd5e1"},
    )

    fig.suptitle("INA219 energy comparison", fontweight="bold")
    plt.tight_layout(rect=(0, 0, 1, 0.94))
    output = PICS_DIR / f"result_energy_comparison_{PLOT_DATE}.png"
    plt.savefig(output)
    plt.close()
    return output


def plot_communication_volume() -> Path:
    labels = ["Fixed baseline\n50 Hz", "Adaptive\n40 Hz"]
    samples = [1250, 1000]
    payload = [2270, 2270]
    colors = ["#64748b", "#0f766e"]

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.5), dpi=160)

    bars = axes[0].bar(labels, samples, color=colors)
    set_common_style(axes[0], "Local samples represented", "samples / 5 windows")
    annotate_bars(axes[0], bars, "{:.0f}")
    axes[0].text(
        0.06,
        0.88,
        "-20% local samples",
        transform=axes[0].transAxes,
        color="#334155",
        fontsize=9,
    )

    bars = axes[1].bar(labels, payload, color=colors)
    set_common_style(axes[1], "MQTT aggregate payload", "bytes / 5 windows")
    annotate_bars(axes[1], bars, "{:.0f}")
    axes[1].text(
        0.06,
        0.88,
        "same aggregate count\none message/window",
        transform=axes[1].transAxes,
        color="#334155",
        fontsize=9,
    )

    fig.suptitle("Communication volume: fixed vs adaptive", fontweight="bold")
    plt.tight_layout(rect=(0, 0, 1, 0.94))
    output = PICS_DIR / f"result_communication_volume_{PLOT_DATE}.png"
    plt.savefig(output)
    plt.close()
    return output


def plot_mqtt_latency() -> Path:
    summary = read_json("mqtt_summary_2026-04-18_listener.json")
    metrics = [
        ("Listener", summary["listener_latency_us"]),
        ("End-to-end", summary["end_to_end_latency_us"]),
        ("Edge delay", summary["edge_delay_us"]),
    ]

    labels = [name for name, _ in metrics]
    avg_ms = [metric["avg"] / 1000.0 for _, metric in metrics]
    p95_ms = [metric["p95"] / 1000.0 for _, metric in metrics]
    x = range(len(labels))
    width = 0.36

    fig, ax = plt.subplots(figsize=(9.5, 4.8), dpi=160)
    bars_avg = ax.bar([i - width / 2 for i in x], avg_ms, width, label="avg", color="#0f766e")
    bars_p95 = ax.bar([i + width / 2 for i in x], p95_ms, width, label="p95", color="#f59e0b")
    ax.set_xticks(list(x), labels)
    set_common_style(ax, "MQTT latency summary", "milliseconds")
    annotate_bars(ax, bars_avg, "{:.0f}")
    annotate_bars(ax, bars_p95, "{:.0f}")
    ax.legend()
    ax.text(
        0.02,
        0.88,
        f"{summary['messages_received']} messages, {summary['unique_windows']} windows",
        transform=ax.transAxes,
        color="#334155",
        fontsize=9,
    )

    plt.tight_layout()
    output = PICS_DIR / f"result_mqtt_latency_{PLOT_DATE}.png"
    plt.savefig(output)
    plt.close()
    return output


def plot_anomaly_filters() -> Path:
    rows = []
    with (SUMMARY_DIR / "anomaly_filter_metrics_2026-04-21.csv").open(
        newline="", encoding="utf-8"
    ) as handle:
        reader = csv.DictReader(handle)
        rows = list(reader)

    grouped: dict[str, list[dict[str, str]]] = {"zscore": [], "hampel": []}
    for row in rows:
        grouped[row["filter"]].append(row)

    colors = {"zscore": "#0f766e", "hampel": "#b45309"}
    labels = {"zscore": "Z-score", "hampel": "Hampel"}

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.6), dpi=160)

    for filter_name, filter_rows in grouped.items():
        filter_rows.sort(key=lambda item: float(item["anomaly_probability"]))
        x = [float(row["anomaly_probability"]) * 100.0 for row in filter_rows]
        tpr = [float(row["tpr"]) * 100.0 for row in filter_rows]
        fpr = [float(row["fpr"]) * 100.0 for row in filter_rows]
        reduction = [float(row["mean_error_reduction_percent"]) for row in filter_rows]

        axes[0].plot(x, tpr, marker="o", color=colors[filter_name], label=f"{labels[filter_name]} TPR")
        axes[0].plot(
            x,
            fpr,
            marker="x",
            linestyle="--",
            color=colors[filter_name],
            label=f"{labels[filter_name]} FPR",
        )
        axes[1].plot(
            x,
            reduction,
            marker="o",
            color=colors[filter_name],
            label=labels[filter_name],
        )

    set_common_style(axes[0], "Detection performance", "rate (%)")
    axes[0].set_xlabel("Injected anomaly probability (%)")
    axes[0].set_ylim(bottom=0)
    axes[0].legend(fontsize=8)

    set_common_style(axes[1], "Mean error reduction", "reduction (%)")
    axes[1].set_xlabel("Injected anomaly probability (%)")
    axes[1].axhline(0.0, color="#1f2937", linewidth=0.8, alpha=0.45)
    axes[1].legend(fontsize=8)

    fig.suptitle("Anomaly filter evaluation", fontweight="bold")
    plt.tight_layout(rect=(0, 0, 1, 0.94))
    output = PICS_DIR / f"result_anomaly_filters_{PLOT_DATE}.png"
    plt.savefig(output)
    plt.close()
    return output


def main() -> None:
    PICS_DIR.mkdir(parents=True, exist_ok=True)
    outputs = [
        plot_energy(),
        plot_communication_volume(),
        plot_mqtt_latency(),
        plot_anomaly_filters(),
    ]
    for output in outputs:
        print(f"Wrote {output.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
