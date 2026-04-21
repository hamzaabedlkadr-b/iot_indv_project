from __future__ import annotations

import argparse
import csv
import json
import math
import time
from dataclasses import dataclass
from pathlib import Path
from statistics import mean, median
from typing import Iterable


SEED = 0x5A17C3E1
SAMPLE_RATE_HZ = 40.0
WINDOW_SECONDS = 5.0
SAMPLE_COUNT = int(SAMPLE_RATE_HZ * WINDOW_SECONDS)
NOISE_SIGMA = 0.2
ANOMALY_MIN = 5.0
ANOMALY_MAX = 15.0
ACTIVE_POWER_MW = 552.677530
EXPECTED_DOMINANT_HZ = 5.0
OVERSAMPLING_FACTOR = 8.0
RATE_STEP_HZ = 5.0
MIN_RATE_HZ = 20.0
MAX_RATE_HZ = 50.0


@dataclass
class Trial:
    clean: list[float]
    contaminated: list[float]
    anomaly_mask: list[bool]


class XorShift32:
    def __init__(self, seed: int) -> None:
        self.state = seed & 0xFFFFFFFF

    def next_u32(self) -> int:
        value = self.state or SEED
        value ^= (value << 13) & 0xFFFFFFFF
        value ^= value >> 17
        value ^= (value << 5) & 0xFFFFFFFF
        self.state = value & 0xFFFFFFFF
        return self.state

    def uniform(self) -> float:
        return (self.next_u32() & 0x00FFFFFF) / 16777216.0

    def gaussian_like(self, sigma: float) -> float:
        return (sum(self.uniform() for _ in range(12)) - 6.0) * sigma


def base_signal(time_seconds: float) -> float:
    term_1 = 2.0 * math.sin(2.0 * math.pi * 3.0 * time_seconds)
    term_2 = 4.0 * math.sin(2.0 * math.pi * 5.0 * time_seconds)
    return term_1 + term_2


def generate_trial(rng: XorShift32, anomaly_probability: float, trial_index: int) -> Trial:
    clean: list[float] = []
    contaminated: list[float] = []
    anomaly_mask: list[bool] = []
    start_sample = trial_index * SAMPLE_COUNT

    for sample_index in range(SAMPLE_COUNT):
        time_seconds = (start_sample + sample_index) / SAMPLE_RATE_HZ
        clean_value = base_signal(time_seconds)
        value = clean_value + rng.gaussian_like(NOISE_SIGMA)
        injected = rng.uniform() < anomaly_probability

        if injected:
            magnitude = ANOMALY_MIN + (rng.uniform() * (ANOMALY_MAX - ANOMALY_MIN))
            sign = -1.0 if rng.uniform() < 0.5 else 1.0
            value += sign * magnitude

        clean.append(clean_value)
        contaminated.append(value)
        anomaly_mask.append(injected)

    return Trial(clean=clean, contaminated=contaminated, anomaly_mask=anomaly_mask)


def stdev(values: list[float]) -> float:
    if not values:
        return 0.0
    avg = mean(values)
    return math.sqrt(sum((value - avg) ** 2 for value in values) / len(values))


def zscore_filter(values: list[float], threshold: float) -> tuple[list[bool], list[float]]:
    avg = mean(values)
    sigma = stdev(values)
    replacement = median(values)
    flags = [abs(value - avg) > threshold * sigma if sigma > 0.0 else False for value in values]
    filtered = [replacement if flag else value for value, flag in zip(values, flags)]
    return flags, filtered


def local_window(values: list[float], index: int, window_size: int) -> list[float]:
    radius = window_size // 2
    start = max(0, index - radius)
    end = min(len(values), index + radius + 1)
    return values[start:end]


def hampel_filter(values: list[float], window_size: int, threshold: float) -> tuple[list[bool], list[float]]:
    flags: list[bool] = []
    filtered: list[float] = []

    for index, value in enumerate(values):
        window = local_window(values, index, window_size)
        local_median = median(window)
        mad = median([abs(item - local_median) for item in window])
        robust_sigma = 1.4826 * mad
        flag = abs(value - local_median) > threshold * robust_sigma if robust_sigma > 0.0 else False
        flags.append(flag)
        filtered.append(local_median if flag else value)

    return flags, filtered


def dft_dominant_frequency(values: list[float], sample_rate_hz: float) -> float:
    sample_mean = mean(values)
    best_frequency = 0.0
    best_magnitude = -1.0
    sample_count = len(values)

    for bin_index in range(1, (sample_count // 2) + 1):
        real = 0.0
        imag = 0.0
        angle_step = 2.0 * math.pi * bin_index / sample_count

        for sample_index, value in enumerate(values):
            centered = value - sample_mean
            angle = angle_step * sample_index
            real += centered * math.cos(angle)
            imag -= centered * math.sin(angle)

        magnitude = math.sqrt((real * real) + (imag * imag)) / sample_count
        if magnitude > best_magnitude:
            best_magnitude = magnitude
            best_frequency = (bin_index * sample_rate_hz) / sample_count

    return best_frequency


def adaptive_rate_for(dominant_frequency_hz: float) -> float:
    if dominant_frequency_hz <= 0.0:
        return MAX_RATE_HZ
    proposed = round((dominant_frequency_hz * OVERSAMPLING_FACTOR) / RATE_STEP_HZ) * RATE_STEP_HZ
    return min(MAX_RATE_HZ, max(MIN_RATE_HZ, proposed))


def percent_reduction(before: float, after: float) -> float:
    if before <= 0.0:
        return 0.0
    return ((before - after) / before) * 100.0


def count_flags(predicted: Iterable[bool], actual: Iterable[bool]) -> tuple[int, int, int, int]:
    true_positive = false_positive = true_negative = false_negative = 0
    for predicted_flag, actual_flag in zip(predicted, actual):
        if predicted_flag and actual_flag:
            true_positive += 1
        elif predicted_flag and not actual_flag:
            false_positive += 1
        elif not predicted_flag and actual_flag:
            false_negative += 1
        else:
            true_negative += 1
    return true_positive, false_positive, true_negative, false_negative


def evaluate_filter(
    trials: list[Trial],
    filter_name: str,
    anomaly_probability: float,
    window_size: int,
    threshold: float,
) -> dict[str, float | int | str]:
    true_positive = false_positive = true_negative = false_negative = 0
    contaminated_mean_errors: list[float] = []
    filtered_mean_errors: list[float] = []
    contaminated_dom_errors: list[float] = []
    filtered_dom_errors: list[float] = []
    contaminated_wrong_dom = 0
    filtered_wrong_dom = 0
    contaminated_rates: list[float] = []
    filtered_rates: list[float] = []
    execution_times_us: list[float] = []

    for trial in trials:
        start = time.perf_counter()
        if filter_name == "zscore":
            flags, filtered = zscore_filter(trial.contaminated, threshold)
            effective_window = SAMPLE_COUNT
        elif filter_name == "hampel":
            flags, filtered = hampel_filter(trial.contaminated, window_size, threshold)
            effective_window = window_size
        else:
            raise ValueError(f"Unsupported filter: {filter_name}")
        elapsed_us = (time.perf_counter() - start) * 1_000_000.0
        execution_times_us.append(elapsed_us)

        tp, fp, tn, fn = count_flags(flags, trial.anomaly_mask)
        true_positive += tp
        false_positive += fp
        true_negative += tn
        false_negative += fn

        clean_mean = mean(trial.clean)
        contaminated_mean_errors.append(abs(mean(trial.contaminated) - clean_mean))
        filtered_mean_errors.append(abs(mean(filtered) - clean_mean))

        contaminated_dom = dft_dominant_frequency(trial.contaminated, SAMPLE_RATE_HZ)
        filtered_dom = dft_dominant_frequency(filtered, SAMPLE_RATE_HZ)
        contaminated_dom_errors.append(abs(contaminated_dom - EXPECTED_DOMINANT_HZ))
        filtered_dom_errors.append(abs(filtered_dom - EXPECTED_DOMINANT_HZ))
        contaminated_wrong_dom += int(abs(contaminated_dom - EXPECTED_DOMINANT_HZ) > 0.01)
        filtered_wrong_dom += int(abs(filtered_dom - EXPECTED_DOMINANT_HZ) > 0.01)
        contaminated_rates.append(adaptive_rate_for(contaminated_dom))
        filtered_rates.append(adaptive_rate_for(filtered_dom))

    tpr_denominator = true_positive + false_negative
    fpr_denominator = false_positive + true_negative
    avg_execution_us = mean(execution_times_us)
    estimated_energy_uwh = ACTIVE_POWER_MW * (avg_execution_us / 1_000_000.0) / 3600.0 * 1000.0
    filter_delay_s = ((effective_window // 2) / SAMPLE_RATE_HZ) if filter_name == "hampel" else WINDOW_SECONDS
    memory_bytes = effective_window * 4

    return {
        "filter": filter_name,
        "anomaly_probability": anomaly_probability,
        "window_size": effective_window,
        "threshold": threshold,
        "trials": len(trials),
        "samples_per_trial": SAMPLE_COUNT,
        "tpr": true_positive / tpr_denominator if tpr_denominator else 0.0,
        "fpr": false_positive / fpr_denominator if fpr_denominator else 0.0,
        "true_positive": true_positive,
        "false_positive": false_positive,
        "true_negative": true_negative,
        "false_negative": false_negative,
        "contaminated_mean_error": mean(contaminated_mean_errors),
        "filtered_mean_error": mean(filtered_mean_errors),
        "mean_error_reduction_percent": percent_reduction(
            mean(contaminated_mean_errors), mean(filtered_mean_errors)
        ),
        "contaminated_fft_error_hz": mean(contaminated_dom_errors),
        "filtered_fft_error_hz": mean(filtered_dom_errors),
        "contaminated_wrong_dominant_windows": contaminated_wrong_dom,
        "filtered_wrong_dominant_windows": filtered_wrong_dom,
        "contaminated_adaptive_rate_hz": mean(contaminated_rates),
        "filtered_adaptive_rate_hz": mean(filtered_rates),
        "avg_execution_us": avg_execution_us,
        "estimated_filter_energy_uwh": estimated_energy_uwh,
        "filter_delay_s": filter_delay_s,
        "state_memory_bytes": memory_bytes,
    }


def write_csv(path: Path, rows: list[dict[str, float | int | str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def format_percent(value: float) -> str:
    return f"{value * 100.0:.2f}%"


def build_markdown(main_rows: list[dict[str, float | int | str]],
                   tradeoff_rows: list[dict[str, float | int | str]]) -> str:
    lines = [
        "# Anomaly Filter Evaluation",
        "",
        "This deterministic host-side evaluation mirrors the firmware signal model:",
        "",
        "- clean signal: `2*sin(2*pi*3*t) + 4*sin(2*pi*5*t)`",
        "- noise: near-Gaussian baseline noise with `sigma = 0.2`",
        "- anomaly process: sparse spikes with `p = 1%, 5%, 10%` and magnitude `+/-U(5, 15)`",
        "- window: `5 s` at the adaptive operating rate, so `200` samples per window",
        "",
        "The energy column is an active-power estimate using the measured adaptive INA219 average power. It is not a separate physical INA219 run for each filter, but it gives a consistent per-window cost estimate for the extra CPU work.",
        "",
        "## Detection And Signal Quality",
        "",
        "| Filter | p | Window | TPR | FPR | Mean error reduction | FFT error before | FFT error after | Adaptive rate after | Exec time | Est. energy |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]

    for row in main_rows:
        lines.append(
            "| {filter} | {p:.0%} | {window} | {tpr} | {fpr} | {reduction:.2f}% | "
            "`{before:.3f} Hz` | `{after:.3f} Hz` | `{rate:.1f} Hz` | `{exec_us:.1f} us` | `{energy:.4f} uWh` |".format(
                filter=row["filter"],
                p=float(row["anomaly_probability"]),
                window=int(row["window_size"]),
                tpr=format_percent(float(row["tpr"])),
                fpr=format_percent(float(row["fpr"])),
                reduction=float(row["mean_error_reduction_percent"]),
                before=float(row["contaminated_fft_error_hz"]),
                after=float(row["filtered_fft_error_hz"]),
                rate=float(row["filtered_adaptive_rate_hz"]),
                exec_us=float(row["avg_execution_us"]),
                energy=float(row["estimated_filter_energy_uwh"]),
            )
        )

    lines.extend(
        [
            "",
            "## Hampel Window Trade-Off",
            "",
            "| p | Hampel window | TPR | FPR | Delay added | State memory | Exec time | Mean error reduction |",
            "|---:|---:|---:|---:|---:|---:|---:|---:|",
        ]
    )

    for row in tradeoff_rows:
        lines.append(
            "| {p:.0%} | {window} | {tpr} | {fpr} | `{delay:.3f} s` | `{memory} B` | `{exec_us:.1f} us` | {reduction:.2f}% |".format(
                p=float(row["anomaly_probability"]),
                window=int(row["window_size"]),
                tpr=format_percent(float(row["tpr"])),
                fpr=format_percent(float(row["fpr"])),
                delay=float(row["filter_delay_s"]),
                memory=int(row["state_memory_bytes"]),
                exec_us=float(row["avg_execution_us"]),
                reduction=float(row["mean_error_reduction_percent"]),
            )
        )

    lines.extend(
        [
            "",
            "## Interpretation",
            "",
            "- `Z-score` is the cheapest option and performs well for this low-frequency sinusoidal signal, but its detection rate drops as spike contamination rises because the window mean and standard deviation are affected by outliers.",
            "- `Hampel` gives a tunable robustness/delay trade-off: larger windows reduce false positives and improve mean-error reduction, but they add centered-filter delay and require more state.",
            "- The dominant FFT component remains close to `5 Hz` after filtering, so the adaptive policy remains stable at `40 Hz` instead of being pulled toward a higher noisy bin.",
            "- Larger Hampel windows improve statistical context but increase centered-filter delay by roughly `window/2` samples and increase memory linearly.",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Evaluate Z-score and Hampel anomaly filters.")
    parser.add_argument("--trials", type=int, default=100)
    parser.add_argument("--output-dir", type=Path, default=Path("source/results/summaries"))
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    rng = XorShift32(SEED)
    probabilities = [0.01, 0.05, 0.10]
    main_rows: list[dict[str, float | int | str]] = []
    tradeoff_rows: list[dict[str, float | int | str]] = []

    for probability in probabilities:
        trials = [generate_trial(rng, probability, trial_index) for trial_index in range(args.trials)]
        main_rows.append(evaluate_filter(trials, "zscore", probability, SAMPLE_COUNT, 2.0))
        main_rows.append(evaluate_filter(trials, "hampel", probability, 11, 2.0))

        for window_size in [5, 11, 21, 41]:
            tradeoff_rows.append(evaluate_filter(trials, "hampel", probability, window_size, 2.0))

    args.output_dir.mkdir(parents=True, exist_ok=True)
    metrics_csv = args.output_dir / "anomaly_filter_metrics_2026-04-21.csv"
    tradeoff_csv = args.output_dir / "anomaly_filter_window_tradeoff_2026-04-21.csv"
    json_path = args.output_dir / "anomaly_filter_evaluation_2026-04-21.json"
    md_path = args.output_dir / "anomaly_filter_evaluation_2026-04-21.md"

    write_csv(metrics_csv, main_rows)
    write_csv(tradeoff_csv, tradeoff_rows)
    json_path.write_text(
        json.dumps(
            {
                "seed": SEED,
                "sample_rate_hz": SAMPLE_RATE_HZ,
                "window_seconds": WINDOW_SECONDS,
                "samples_per_window": SAMPLE_COUNT,
                "metrics": main_rows,
                "window_tradeoff": tradeoff_rows,
            },
            indent=2,
        ),
        encoding="utf-8",
    )
    md_path.write_text(build_markdown(main_rows, tradeoff_rows), encoding="utf-8")
    print(f"Wrote {md_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
