#!/usr/bin/env python3
"""Generate grouped SVG bar charts from the benchmark CSV."""

import argparse
import csv
import html
import math
from pathlib import Path


ALGORITHMS = ("insertion", "quick", "tree")
COLORS = {"insertion": "#0072B2", "quick": "#D55E00", "tree": "#009E73"}
LABELS = {"insertion": "Insertion", "quick": "Quick", "tree": "Tree"}
SHAPES = ("random", "sorted", "reverse", "duplicate_heavy")
SHAPE_LABELS = {
    "random": "무작위",
    "sorted": "정렬됨",
    "reverse": "역순",
    "duplicate_heavy": "중복 다수",
}
METRICS = {
    "time_ms": ("실행 시간", "시간 (ms)"),
    "comparisons": ("비교 횟수", "비교 횟수"),
    "moves": ("이동 횟수", "이동 횟수"),
    "max_recursion_depth": ("최대 재귀 깊이", "재귀 깊이"),
}
def read_results(csv_path):
    with csv_path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    required = {
        "experiment", "input_shape", "array_size", "algorithm", *METRICS
    }
    if not rows or not required.issubset(rows[0]):
        raise ValueError(f"{csv_path} is missing benchmark columns or rows")
    return rows


def read_small_array_results(csv_path):
    with csv_path.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    required = {"array_size", "algorithm", "time_us"}
    if not rows or not required.issubset(rows[0]):
        raise ValueError(f"{csv_path} is missing small-array columns or rows")

    sizes = sorted({int(row["array_size"]) for row in rows})
    series = {algorithm: [] for algorithm in ALGORITHMS}
    for size in sizes:
        size_rows = {
            row["algorithm"]: float(row["time_us"])
            for row in rows
            if int(row["array_size"]) == size
        }
        if set(size_rows) != set(ALGORITHMS):
            raise ValueError(f"{csv_path} must include all algorithms for n={size}")
        for algorithm in ALGORITHMS:
            series[algorithm].append(size_rows[algorithm])
    return sizes, series


def format_value(value, metric):
    if metric == "time_ms":
        return f"{value:.3f}"
    return f"{int(value):,}"


def nice_ticks(maximum):
    step = maximum / 5.0
    magnitude = 10 ** math.floor(math.log10(step))
    normalized = step / magnitude
    factor = 1 if normalized <= 1 else 2 if normalized <= 2 else 5 if normalized <= 5 else 10
    step = factor * magnitude
    ticks = [step * index for index in range(6)]
    return ticks, step * 5


def log_ticks(minimum, maximum):
    first_power = math.floor(math.log10(minimum))
    last_power = math.ceil(math.log10(maximum))
    return [10.0**power for power in range(first_power, last_power + 1)
            if minimum <= 10.0**power <= maximum]


def render_chart(title, y_label, categories, values, metric, output_path, logarithmic):
    width, height = 1100, 560
    left, right, top, bottom = 95, 32, 86, 120
    plot_width, plot_height = width - left - right, height - top - bottom
    maximum = max(values.values(), default=0.0)
    positive = [value for value in values.values() if value > 0]

    if logarithmic and positive:
        minimum = min(positive) / 10.0
        upper = max(positive)
        ticks = log_ticks(minimum, upper)
        log_min, log_max = math.log10(minimum), math.log10(upper)
        if log_min == log_max:
            log_min -= 1

        def y_position(value):
            if value <= 0:
                return top + plot_height
            ratio = (math.log10(value) - log_min) / (log_max - log_min)
            return top + plot_height * (1.0 - ratio)

        axis_max = upper
    else:
        ticks, axis_max = nice_ticks(maximum if maximum > 0 else 1.0)

        def y_position(value):
            return top + plot_height * (1.0 - value / axis_max)

    pieces = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" role="img" aria-label="{html.escape(title)}">',
        '<style>text{font-family:"DejaVu Sans",sans-serif;fill:#202b33}'
        '.grid{stroke:#dce3e6;stroke-width:1}.axis{stroke:#53636c;stroke-width:1.2}'
        '.value{font-size:11px}.tick{font-size:12px}.category{font-size:14px}'
        '.legend{font-size:13px}.title{font-size:21px;font-weight:700}'
        '.ylabel{font-size:13px}</style>',
        f'<text class="title" x="{width / 2}" y="36" text-anchor="middle">{html.escape(title)}</text>',
    ]

    for tick in ticks:
        y = y_position(tick)
        if y < top - 1 or y > top + plot_height + 1:
            continue
        pieces.append(f'<line class="grid" x1="{left}" y1="{y:.1f}" x2="{width-right}" y2="{y:.1f}"/>')
        label = format_value(tick, metric)
        pieces.append(f'<text class="tick" x="{left-12}" y="{y+4:.1f}" text-anchor="end">{label}</text>')

    baseline = top + plot_height
    pieces.extend([
        f'<line class="axis" x1="{left}" y1="{top}" x2="{left}" y2="{baseline}"/>',
        f'<line class="axis" x1="{left}" y1="{baseline}" x2="{width-right}" y2="{baseline}"/>',
        f'<text class="ylabel" x="22" y="{top + plot_height / 2}" '
        f'transform="rotate(-90 22 {top + plot_height / 2})" text-anchor="middle">{html.escape(y_label)}</text>',
    ])

    slot_width = plot_width / len(categories)
    bar_width = min(52.0, slot_width * 0.19)
    gap = bar_width * 0.16
    group_width = len(ALGORITHMS) * bar_width + (len(ALGORITHMS) - 1) * gap
    for category_index, category in enumerate(categories):
        center = left + slot_width * (category_index + 0.5)
        start = center - group_width / 2
        for algorithm_index, algorithm in enumerate(ALGORITHMS):
            value = values[(category, algorithm)]
            x = start + algorithm_index * (bar_width + gap)
            y = y_position(value)
            bar_height = max(0.0, baseline - y)
            if value > 0:
                pieces.append(
                    f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_width:.1f}" '
                    f'height="{bar_height:.1f}" fill="{COLORS[algorithm]}" rx="1"/>'
                )
            label_y = max(top + 13, y - 6)
            value_label = format_value(value, metric)
            pieces.append(
                f'<text class="value" x="{x + bar_width / 2:.1f}" y="{label_y:.1f}" '
                f'text-anchor="middle">{value_label}</text>'
            )
        pieces.append(
            f'<text class="category" x="{center:.1f}" y="{baseline + 25}" '
            f'text-anchor="middle">{html.escape(category)}</text>'
        )

    legend_y = height - 30
    legend_width = 130
    legend_start = (width - legend_width * len(ALGORITHMS)) / 2
    for index, algorithm in enumerate(ALGORITHMS):
        x = legend_start + index * legend_width
        pieces.append(f'<rect x="{x:.1f}" y="{legend_y-13}" width="14" height="14" fill="{COLORS[algorithm]}"/>')
        pieces.append(f'<text class="legend" x="{x+21:.1f}" y="{legend_y}">{LABELS[algorithm]}</text>')
    if logarithmic:
        pieces.append(
            f'<text class="tick" x="{left}" y="{baseline+48}" text-anchor="start">'
            '0은 기준선에 표시 (로그축의 양수 구간 밖)</text>'
        )
    pieces.append('</svg>')
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(pieces) + "\n", encoding="utf-8")


def render_line_chart(title, y_label, categories, series, output_path):
    width, height = 1100, 560
    left, right, top, bottom = 95, 32, 86, 120
    plot_width, plot_height = width - left - right, height - top - bottom
    maximum = max(max(values) for values in series.values())
    ticks, axis_max = nice_ticks(maximum * 1.1)
    pieces = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" role="img" aria-label="{html.escape(title)}">',
        '<style>text{font-family:"DejaVu Sans",sans-serif;fill:#202b33}'
        '.grid{stroke:#dce3e6;stroke-width:1}.axis{stroke:#53636c;stroke-width:1.2}'
        '.tick{font-size:12px}.category{font-size:14px}.legend{font-size:13px}'
        '.title{font-size:21px;font-weight:700}.ylabel{font-size:13px}</style>',
        f'<text class="title" x="{width / 2}" y="36" text-anchor="middle">{html.escape(title)}</text>',
    ]

    def y_position(value):
        return top + plot_height * (1.0 - value / axis_max)

    for tick in ticks:
        y = y_position(tick)
        pieces.append(f'<line class="grid" x1="{left}" y1="{y:.1f}" x2="{width-right}" y2="{y:.1f}"/>')
        pieces.append(f'<text class="tick" x="{left-12}" y="{y+4:.1f}" text-anchor="end">{tick:.4f}</text>')

    baseline = top + plot_height
    pieces.extend([
        f'<line class="axis" x1="{left}" y1="{top}" x2="{left}" y2="{baseline}"/>',
        f'<line class="axis" x1="{left}" y1="{baseline}" x2="{width-right}" y2="{baseline}"/>',
        f'<text class="ylabel" x="22" y="{top + plot_height / 2}" '
        f'transform="rotate(-90 22 {top + plot_height / 2})" text-anchor="middle">{html.escape(y_label)}</text>',
    ])

    slot_width = plot_width / len(categories)
    for algorithm in ALGORITHMS:
        points = [
            (left + slot_width * (index + 0.5), y_position(value))
            for index, value in enumerate(series[algorithm])
        ]
        point_list = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
        pieces.append(
            f'<polyline points="{point_list}" fill="none" stroke="{COLORS[algorithm]}" stroke-width="3"/>'
        )
        for x, y in points:
            pieces.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{COLORS[algorithm]}"/>')

    for index, category in enumerate(categories):
        x = left + slot_width * (index + 0.5)
        pieces.append(f'<text class="category" x="{x:.1f}" y="{baseline+25}" text-anchor="middle">{html.escape(category)}</text>')

    legend_y = height - 30
    legend_width = 130
    legend_start = (width - legend_width * len(ALGORITHMS)) / 2
    for index, algorithm in enumerate(ALGORITHMS):
        x = legend_start + index * legend_width
        pieces.append(f'<line x1="{x:.1f}" y1="{legend_y-6}" x2="{x+16:.1f}" y2="{legend_y-6}" stroke="{COLORS[algorithm]}" stroke-width="3"/>')
        pieces.append(f'<text class="legend" x="{x+21:.1f}" y="{legend_y}">{LABELS[algorithm]}</text>')
    pieces.append('</svg>')
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(pieces) + "\n", encoding="utf-8")


def generate(rows, output_dir):
    shape_rows = [row for row in rows if row["experiment"] == "input_shape"]
    size_rows = [row for row in rows if row["experiment"] == "random_size"]
    for metric, (metric_label, y_label) in METRICS.items():
        shape_values = {
            (SHAPE_LABELS[row["input_shape"]], row["algorithm"]): float(row[metric])
            for row in shape_rows
        }
        for scale in ("linear", "log"):
            render_chart(
                f"입력 형태별 {metric_label} ({scale} 축)", y_label,
                [SHAPE_LABELS[shape] for shape in SHAPES], shape_values,
                metric, output_dir / f"shape-{metric}-{scale}.svg", scale == "log",
            )

    sizes = sorted({int(row["array_size"]) for row in size_rows})
    categories = [str(size) for size in sizes]
    for metric in ("comparisons", "time_ms"):
        metric_label, y_label = METRICS[metric]
        values = {
            (row["array_size"], row["algorithm"]): float(row[metric])
            for row in size_rows
        }
        for scale in ("linear", "log"):
            render_chart(
                f"무작위 입력 크기별 {metric_label} ({scale} 축)", y_label,
                categories, values, metric,
                output_dir / f"random-size-{metric}-{scale}.svg", scale == "log",
            )

    baseline_rows = [
        row for row in shape_rows
        if row["input_shape"] == "random" and int(row["array_size"]) == 2000
    ]
    if {row["algorithm"] for row in baseline_rows} != set(ALGORITHMS):
        raise ValueError("results.csv must include the n=2000 random input rows")

    summary_category = ["n=2,000"]
    for metric, metric_label, unit in (
        ("time_ms", "실행 시간", "ms"),
        ("auxiliary_bytes", "보조 공간", "bytes"),
    ):
        values_by_algorithm = {
            row["algorithm"]: float(row[metric]) for row in baseline_rows
        }
        values = {
            (summary_category[0], algorithm): value
            for algorithm, value in values_by_algorithm.items()
        }
        render_chart(
            f"n=2,000 무작위 입력의 {metric_label}", unit, summary_category,
            values, metric, output_dir / f"summary-{metric}.svg", False,
        )

    small_array_sizes, small_array_times = read_small_array_results(
        Path("report/small-array-results.csv")
    )
    render_line_chart(
        "작은 배열 크기별 실행 시간", "시간 (µs)",
        [str(size) for size in small_array_sizes], small_array_times,
        output_dir / "small-array-time.svg",
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", nargs="?", type=Path, default=Path("report/results.csv"))
    parser.add_argument("--output-dir", type=Path, default=Path("report/tools/charts"))
    args = parser.parse_args()
    generate(read_results(args.csv), args.output_dir)
    print(f"SVG charts written to {args.output_dir}")


if __name__ == "__main__":
    main()
