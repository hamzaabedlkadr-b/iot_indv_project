"""Generate static plots for the firmware input signal profiles.

The equations and constants mirror source/firmware/esp32_node/include/project_config.h
and source/firmware/esp32_node/components/signal_input/signal_input.c so the plots
are reproducible without flashing the board.
"""

from __future__ import annotations

import csv
import math
from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[2]
PICS_DIR = ROOT / "source" / "pics"
SUMMARY_DIR = ROOT / "source" / "results" / "summaries"

PLOT_DATE = "2026-04-22"
PLOT_RATE_HZ = 200.0
PLOT_DURATION_S = 2.0

BASE_AMPLITUDE_3HZ = 2.0
BASE_FREQUENCY_3HZ = 3.0
BASE_AMPLITUDE_5HZ = 4.0
BASE_FREQUENCY_5HZ = 5.0

NOISE_SIGMA = 0.2
ANOMALY_PROBABILITY = 0.02
ANOMALY_MAGNITUDE_MIN = 5.0
ANOMALY_MAGNITUDE_MAX = 15.0
RANDOM_SEED = 0x5A17C3E1

PROFILES = ("clean_reference", "noisy_reference", "anomaly_stress")


def xorshift32_next(state: int) -> int:
    """Match the lightweight firmware pseudo-random generator."""
    state ^= (state << 13) & 0xFFFFFFFF
    state ^= (state >> 17) & 0xFFFFFFFF
    state ^= (state << 5) & 0xFFFFFFFF
    return state & 0xFFFFFFFF


def random_uniform(state: int) -> tuple[int, float]:
    state = xorshift32_next(state)
    return state, state / 4_294_967_295.0


def gaussian_like_noise(state: int) -> tuple[int, float]:
    total = 0.0
    for _ in range(12):
        state, value = random_uniform(state)
        total += value
    return state, (total - 6.0) * NOISE_SIGMA


def anomaly_sample(state: int) -> tuple[int, float, bool]:
    state, trigger = random_uniform(state)
    if trigger >= ANOMALY_PROBABILITY:
        return state, 0.0, False

    state, magnitude_seed = random_uniform(state)
    magnitude = ANOMALY_MAGNITUDE_MIN + magnitude_seed * (
        ANOMALY_MAGNITUDE_MAX - ANOMALY_MAGNITUDE_MIN
    )

    state, sign_seed = random_uniform(state)
    sign = -1.0 if sign_seed < 0.5 else 1.0
    return state, sign * magnitude, True


def base_signal(t_s: float) -> float:
    return (
        BASE_AMPLITUDE_3HZ * math.sin(2.0 * math.pi * BASE_FREQUENCY_3HZ * t_s)
        + BASE_AMPLITUDE_5HZ * math.sin(2.0 * math.pi * BASE_FREQUENCY_5HZ * t_s)
    )


def time_axis() -> list[float]:
    sample_count = int(PLOT_DURATION_S * PLOT_RATE_HZ) + 1
    return [index / PLOT_RATE_HZ for index in range(sample_count)]


def generate_profile(profile: str, times: Iterable[float]) -> list[dict[str, float | str | int]]:
    state = RANDOM_SEED
    rows: list[dict[str, float | str | int]] = []

    for t_s in times:
        base_value = base_signal(t_s)
        noise = 0.0
        anomaly = 0.0
        injected = False

        if profile in {"noisy_reference", "anomaly_stress"}:
            state, noise = gaussian_like_noise(state)

        if profile == "anomaly_stress":
            state, anomaly, injected = anomaly_sample(state)

        rows.append(
            {
                "profile": profile,
                "time_s": t_s,
                "value": base_value + noise + anomaly,
                "base_value": base_value,
                "noise": noise,
                "anomaly": anomaly,
                "anomaly_injected": int(injected),
            }
        )

    return rows


def write_csv(profile_rows: dict[str, list[dict[str, float | str | int]]]) -> Path:
    SUMMARY_DIR.mkdir(parents=True, exist_ok=True)
    output = SUMMARY_DIR / f"input_signal_samples_{PLOT_DATE}.csv"
    fields = [
        "profile",
        "time_s",
        "value",
        "base_value",
        "noise",
        "anomaly",
        "anomaly_injected",
    ]

    with output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for profile in PROFILES:
            writer.writerows(profile_rows[profile])

    return output


def style_axis(ax, title: str) -> None:
    ax.set_title(title, fontweight="bold")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Signal value")
    ax.grid(True, alpha=0.25)
    ax.axhline(0.0, color="#1f2933", linewidth=0.8, alpha=0.35)


def plot_clean(clean_rows: list[dict[str, float | str | int]]) -> Path:
    PICS_DIR.mkdir(parents=True, exist_ok=True)
    output = PICS_DIR / f"input_signal_clean_reference_{PLOT_DATE}.png"

    times = [float(row["time_s"]) for row in clean_rows]
    values = [float(row["value"]) for row in clean_rows]

    plt.figure(figsize=(11, 4.8), dpi=160)
    ax = plt.gca()
    ax.plot(times, values, color="#155e75", linewidth=2.1)
    style_axis(ax, "Clean input signal: 2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)")
    ax.text(
        0.01,
        0.93,
        "components: 3 Hz and 5 Hz",
        transform=ax.transAxes,
        color="#475569",
        fontsize=9,
    )
    plt.tight_layout()
    plt.savefig(output)
    plt.close()
    return output


def plot_profiles(profile_rows: dict[str, list[dict[str, float | str | int]]]) -> Path:
    output = PICS_DIR / f"input_signal_profiles_{PLOT_DATE}.png"
    colors = {
        "clean_reference": "#155e75",
        "noisy_reference": "#ca8a04",
        "anomaly_stress": "#b91c1c",
    }
    titles = {
        "clean_reference": "Clean reference",
        "noisy_reference": "Noisy reference",
        "anomaly_stress": "Anomaly stress",
    }

    fig, axes = plt.subplots(3, 1, figsize=(11, 8.4), dpi=160, sharex=True)

    for ax, profile in zip(axes, PROFILES):
        rows = profile_rows[profile]
        times = [float(row["time_s"]) for row in rows]
        values = [float(row["value"]) for row in rows]
        ax.plot(times, values, color=colors[profile], linewidth=1.7)
        style_axis(ax, titles[profile])

        if profile == "anomaly_stress":
            anomaly_times = [
                float(row["time_s"]) for row in rows if int(row["anomaly_injected"]) == 1
            ]
            anomaly_values = [
                float(row["value"]) for row in rows if int(row["anomaly_injected"]) == 1
            ]
            ax.scatter(
                anomaly_times,
                anomaly_values,
                color="#111827",
                s=28,
                label="injected spike",
                zorder=3,
            )
            if anomaly_times:
                ax.legend(loc="upper right")

    fig.suptitle("Input signal profiles used by the ESP32 firmware", fontweight="bold")
    plt.tight_layout(rect=(0, 0, 1, 0.97))
    plt.savefig(output)
    plt.close()
    return output


def main() -> None:
    times = time_axis()
    profile_rows = {profile: generate_profile(profile, times) for profile in PROFILES}

    csv_path = write_csv(profile_rows)
    clean_plot = plot_clean(profile_rows["clean_reference"])
    profile_plot = plot_profiles(profile_rows)

    print(f"Wrote {csv_path.relative_to(ROOT)}")
    print(f"Wrote {clean_plot.relative_to(ROOT)}")
    print(f"Wrote {profile_plot.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
