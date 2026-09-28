"""동일 입력으로 정렬 알고리즘을 측정하고 report.md를 생성한다."""

import platform
import random
import statistics
import sys
import time
import tracemalloc
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from sort import SORTS  # noqa: E402


SIZE = 3000
REPEATS = 5
SEED = 20260928


def measure_time(sort, source):
    samples = []
    for _ in range(REPEATS):
        values = source.copy()
        start = time.perf_counter()
        sort(values)
        samples.append(time.perf_counter() - start)
        if values != sorted(source):
            raise AssertionError(f"{sort.__name__} produced an incorrect result")
    return statistics.median(samples)


def measure_peak_bytes(sort, source):
    values = source.copy()
    tracemalloc.start()
    baseline, _ = tracemalloc.get_traced_memory()
    sort(values)
    _, peak = tracemalloc.get_traced_memory()
    tracemalloc.stop()
    if values != sorted(source):
        raise AssertionError(f"{sort.__name__} produced an incorrect result")
    return peak - baseline


def check_stability(sort):
    records = [(2, "first"), (3, "middle"), (2, "second"), (1, "small")]
    result = sort(records, key=lambda record: record[0])
    return result == [(1, "small"), (2, "first"), (2, "second"), (3, "middle")]


def main():
    generator = random.Random(SEED)
    source = [generator.randrange(1_000_000) for _ in range(SIZE)]
    rows = []

    for name, sort in SORTS.items():
        elapsed = measure_time(sort, source)
        peak = statistics.median(
            measure_peak_bytes(sort, source) for _ in range(REPEATS)
        )
        stable = check_stability(sort)
        stability = "안정" if stable else "불안정"
        rows.append((name, elapsed, peak, stability))

    fastest = min(rows, key=lambda row: row[1])[0]
    least_memory = min(rows, key=lambda row: row[2])[0]
    report = [
        "# 정렬 알고리즘 성능 비교",
        "",
        "## 측정 결과",
        "",
        "| 알고리즘 | 평균/최악 시간복잡도 | 중앙 실행 시간 (ms) | 추가 Python 힙 메모리 (bytes) | 안정성 보장 |",
        "| --- | --- | ---: | ---: | --- |",
    ]
    report.extend(
        f"| {name} sort | {complexity} | {elapsed * 1000:.3f} | {peak:.0f} | {stability} |"
        for (name, elapsed, peak, stability), complexity in zip(
            rows, ("O(n²) / O(n²)", "O(n log n) / O(n²)", "O(n log n) / O(n log n)")
        )
    )
    report.extend(
        [
            "",
            "## 측정 방법",
            "",
            f"- Python {platform.python_version()} ({platform.platform()})에서 측정했다.",
            f"- 시드 {SEED}로 만든 동일한 무작위 정수 {SIZE:,}개를 사용했다.",
            f"- 각 알고리즘 실행 시간을 {REPEATS}회 측정해 중앙값을 기록했다. 입력 복사와 결과 검증 시간은 제외했다.",
            "- 추가 메모리는 `tracemalloc`의 입력 준비 이후 Python 힙 peak 증가량이며, 실행 시간 측정과 별도로 측정했다. Python 인터프리터·호출 스택 및 네이티브 메모리는 포함하지 않는다.",
            "- 안정성은 동일 키를 가진 태그 레코드의 상대 순서를 검사했다. 삽입·병합 정렬은 안정 정렬을 보장하고, 퀵 정렬은 보장하지 않는다.",
            "- 시간과 메모리 값은 실행 환경에 따라 달라진다. `make report`로 재측정할 수 있다.",
            "",
            "## 요약",
            "",
            f"- 이 실행에서 가장 빠른 구현: **{fastest} sort**.",
            f"- 이 실행에서 추가 Python 힙 메모리가 가장 적은 구현: **{least_memory} sort**.",
            "- 입력이 작거나 거의 정렬된 경우에는 삽입 정렬의 단순한 구현이 유리할 수 있다. 일반적인 대규모 입력에서는 퀵 정렬과 병합 정렬이 시간 복잡도 면에서 유리하다.",
            "",
        ]
    )
    report_path = ROOT / "report.md"
    report_path.write_text("\n".join(report), encoding="utf-8")
    print(f"Wrote {report_path}")


if __name__ == "__main__":
    main()
