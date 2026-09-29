"""실행: make run-py"""

import statistics
import time
import tracemalloc

from sort import SORTS

ARRAY_SIZE = 2000
REPEATS = 3
STABILITY_INPUT = [(2, "first"), (3, "middle"), (2, "second"), (1, "small")]
STABILITY_EXPECTED = [(1, "small"), (2, "first"), (2, "second"), (3, "middle")]


def make_input():
    state = 20260928
    values = []
    for _ in range(ARRAY_SIZE):
        state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
        values.append(state % 1_000_000)
    return values


def measure_time(sort, source):
    samples = []
    for _ in range(REPEATS):
        values = source.copy()
        started = time.perf_counter()
        sort(values)
        samples.append(time.perf_counter() - started)
    return statistics.median(samples) * 1000


def measure_peak_bytes(sort, source):
    values = source.copy()
    tracemalloc.start()
    try:
        baseline, _ = tracemalloc.get_traced_memory()
        sort(values)
        _, peak = tracemalloc.get_traced_memory()
    finally:
        tracemalloc.stop()
    return peak - baseline


def is_stable(sort):
    values = STABILITY_INPUT.copy()
    sort(values, key=lambda record: record[0])
    return values == STABILITY_EXPECTED


def main():
    source = make_input()
    expected = sorted(source)
    print(f"Python (n={ARRAY_SIZE}, median of {REPEATS} runs)")
    print(f"{'algorithm':<9} {'ms':>7} {'cmp':>10} {'moves':>10} "
          f"{'peak(B)':>8} {'depth':>5} {'sorted':>6} {'stable':>6}")

    for name, sort in SORTS.items():
        metrics = {"comparisons": 0, "moves": 0}
        counted_values = source.copy()
        sort(counted_values, metrics=metrics)
        sorted_ok = counted_values == expected
        elapsed = measure_time(sort, source)
        peak_bytes = measure_peak_bytes(sort, source)
        print(f"{name:<9} {elapsed:7.3f} {metrics['comparisons']:10,d} "
              f"{metrics['moves']:10,d} {peak_bytes:8,d} {0:5d} "
              f"{'yes' if sorted_ok else 'no':>6} "
              f"{'yes' if is_stable(sort) else 'no':>6}")

    print("cmp=element comparisons; moves=element assignments including scratch copies.")
    print("peak(B)=tracemalloc extra-heap peak; iterative implementations have depth 0.")


if __name__ == "__main__":
    main()
