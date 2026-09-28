"""동일 입력으로 정렬 알고리즘을 측정하고 report.md를 생성한다."""

import platform
import random
import statistics
import sys
import time
import tracemalloc
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from sort import SORTS  # noqa: E402


SIZE = 3000
WARMUPS = 1
REPEATS = 7
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
        for _ in range(WARMUPS):
            sort(source.copy())
        elapsed = measure_time(sort, source)
        peak = statistics.median(
            measure_peak_bytes(sort, source) for _ in range(REPEATS)
        )
        stable = check_stability(sort)
        stability = "안정" if stable else "불안정"
        rows.append((name, elapsed, peak, stability))

    fastest = min(rows, key=lambda row: row[1])[0]
    least_memory = min(rows, key=lambda row: row[2])[0]
    time_values = [elapsed * 1000 for _, elapsed, _, _ in rows]
    memory_values = [peak for _, _, peak, _ in rows]
    time_axis_max = math.ceil(max(time_values) / 50) * 50
    memory_axis_max = math.ceil(max(memory_values) / 10_000) * 10_000
    labels = [name.title() for name, _, _, _ in rows]
    complexities = {
        "insertion": "O(n) / O(n²) / O(n²)",
        "quick": "O(n log n) / O(n log n) / O(n²)",
        "merge": "O(n log n) / O(n log n) / O(n log n)",
    }
    report = [
        "# 정렬 알고리즘 성능 비교",
        "",
        "## 알고리즘 설명과 시각화",
        "",
        "### 삽입 정렬 (Insertion sort)",
        "",
        "왼쪽의 정렬된 구간을 유지하면서 다음 원소를 꺼내, 그보다 큰 원소를 오른쪽으로 옮긴 뒤 빈자리에 삽입한다. 거의 정렬됐거나 원소 수가 적으면 빠르게 동작하지만, 역순에 가까운 입력에서는 이동이 많아진다. 최선은 O(n), 평균·최악은 O(n²)이며 제자리에서 안정적으로 정렬한다.",
        "",
        "```mermaid",
        "flowchart LR",
        '    A["입력: 5, 2, 4, 1"] --> B["정렬 구간: 5"]',
        '    B --> C["2를 앞에 삽입: 2, 5 | 4, 1"]',
        '    C --> D["4를 알맞은 곳에 삽입: 2, 4, 5 | 1"]',
        '    D --> E["1을 맨 앞에 삽입: 1, 2, 4, 5"]',
        "```",
        "",
        "### 퀵 정렬 (Quick sort)",
        "",
        "피벗을 정하고 한 번 훑으면서 작은 값·같은 값·큰 값의 세 구간으로 분할한 뒤, 작은 값과 큰 값 구간을 같은 방식으로 정렬한다. 평균은 O(n log n)이지만 분할이 한쪽으로 치우치면 최악 O(n²)이다. 이 구현은 가운데 원소를 피벗으로 쓰며 제자리 분할을 하지만 안정성은 보장하지 않는다.",
        "",
        "```mermaid",
        "flowchart LR",
        '    A["7, 2, 5, 2, 9, 1"] --> B["피벗 5로 분할"]',
        '    B --> C["작은 값: 1, 2, 2"]',
        '    B --> D["같은 값: 5"]',
        '    B --> E["큰 값: 7, 9"]',
        '    C --> F["정렬 결과: 1, 2, 2, 5, 7, 9"]',
        '    D --> F',
        '    E --> F',
        "```",
        "",
        "### 병합 정렬 (Merge sort)",
        "",
        "배열을 반으로 계속 나눠 원소 하나까지 분해한 다음, 이미 정렬된 두 구간을 작은 값부터 골라 합친다. 입력 상태와 무관하게 O(n log n) 시간이 들며, 임시 버퍼를 사용해 O(n) 추가 공간이 필요하다. 같은 키에서는 왼쪽 원소를 먼저 선택하므로 안정 정렬이다.",
        "",
        "```mermaid",
        "flowchart TD",
        '    A["7, 2, 5, 1"] --> B["7, 2"]',
        '    A --> C["5, 1"]',
        '    B --> D["7"]',
        '    B --> E["2"]',
        '    C --> F["5"]',
        '    C --> G["1"]',
        '    D --> H["2, 7"]',
        '    E --> H',
        '    F --> I["1, 5"]',
        '    G --> I',
        '    H --> J["1, 2, 5, 7"]',
        '    I --> J',
        "```",
        "",
        "## 실험 설계",
        "",
        f"Python {platform.python_version()} ({platform.platform()})에서 Python 구현을 비교했다. 시드 `{SEED}`로 생성한 0 이상 1,000,000 미만의 정수 {SIZE:,}개를 세 구현에 똑같이 제공했다. 정렬 전 원본을 복사하므로 각 실행은 같은 순서의 입력에서 시작한다.",
        "",
        f"시간은 준비 실행 {WARMUPS}회를 버린 뒤 `time.perf_counter()`로 {REPEATS}회 측정했다. 각 회차의 입력 복사는 타이머 시작 전에 하고, 정렬 함수 호출 직후 타이머를 멈췄다. 정렬 결과가 Python 내장 `sorted()` 결과와 같은지 확인하는 비용은 시간에 포함하지 않았다. 대표값은 평균 대신 중앙값을 사용해 일시적인 스케줄링 지연의 영향을 줄였다.",
        "",
        f"메모리는 시간 측정과 분리했다. 입력 복사를 먼저 마친 다음 `tracemalloc`을 시작하고, 정렬 도중 기록된 peak에서 시작 시점 메모리를 뺀 값을 {REPEATS}회 구해 중앙값을 썼다. 따라서 표의 값은 Python이 추적하는 추가 힙 메모리이지 프로세스 전체 메모리나 C 구현의 메모리가 아니다.",
        "",
        "안정성은 정렬 키가 같은 항목에 서로 다른 꼬리표를 붙인 작은 입력으로 확인했다. 정렬 후 동등 키 항목의 원래 순서를 유지하면 안정, 그렇지 않으면 불안정으로 표시한다. 안정성은 속도·메모리처럼 양으로 측정하는 값이 아니라 동작 특성이다.",
        "",
        "```mermaid",
        "flowchart LR",
        f'    A["고정 시드 {SEED}"] --> B["동일한 정수 {SIZE:,}개 생성"]',
        '    B --> C["각 정렬에 같은 입력 복사"]',
        f'    C --> D["준비 {WARMUPS}회 후 시간 {REPEATS}회 측정"]',
        '    D --> E["중앙 실행 시간"]',
        f'    C --> F["tracemalloc 메모리 {REPEATS}회 별도 측정"]',
        '    F --> G["중앙 추가 힙 메모리"]',
        '    C --> H["sorted 결과와 대조"]',
        '    I["꼬리표가 있는 같은 키 레코드"] --> J["동일 키의 순서 검사"]',
        '    E --> K["표와 그래프로 report.md 작성"]',
        '    G --> K',
        '    H --> K',
        '    J --> K',
        "```",
        "",
        "## 결과",
        "",
        "| 알고리즘 | 시간복잡도 (최선 / 평균 / 최악) | 중앙 시간 (ms) | 추가 Python 힙 (bytes) | 안정성 |",
        "| --- | --- | ---: | ---: | --- |",
    ]
    report.extend(
        f"| {name.title()} sort | {complexities[name]} | {elapsed * 1000:.3f} | {peak:.0f} | {stability} |"
        for name, elapsed, peak, stability in rows
    )
    report.extend(
        [
            "",
            "### 실행 시간 그래프",
            "",
            "```mermaid",
            "xychart-beta",
            '    title "중앙 실행 시간 (ms)"',
            f'    x-axis [{", ".join(f"\"{label}\"" for label in labels)}]',
            f'    y-axis "ms" 0 --> {time_axis_max}',
            f'    bar [{", ".join(f"{value:.3f}" for value in time_values)}]',
            "```",
            "",
            "### 추가 Python 힙 메모리 그래프",
            "",
            "```mermaid",
            "xychart-beta",
            '    title "추가 Python 힙 메모리 (bytes)"',
            f'    x-axis [{", ".join(f"\"{label}\"" for label in labels)}]',
            f'    y-axis "bytes" 0 --> {memory_axis_max}',
            f'    bar [{", ".join(str(int(value)) for value in memory_values)}]',
            "```",
            "",
            "그래프의 세로축은 각각 0부터 시작한다. 삽입 정렬의 시간은 이 입력에서 다른 두 방법보다 훨씬 길어 그래프 막대가 상대적으로 작게 보인다. 메모리 그래프는 Python 추적 힙만 비교하며, 병합 정렬의 선형 버퍼가 크게 나타난다.",
            "",
            "## 결과 해석",
            "",
            f"- 가장 짧은 중앙 실행 시간은 **{fastest.title()} sort**였고, 가장 적은 추가 Python 힙은 **{least_memory.title()} sort**였다.",
            "- 삽입 정렬은 추가 버퍼가 거의 없지만, 무작위 3,000개 입력에서 이차 시간 증가가 두드러졌다.",
            "- 퀵 정렬은 보통 적은 추가 공간으로 빠르지만, 최악 시간복잡도가 O(n²)이고 안정 정렬이 아니다.",
            "- 병합 정렬은 O(n log n) 시간을 보장하고 안정적이지만, 임시 버퍼 때문에 추가 메모리가 더 크다.",
            "- 이 수치는 한 환경과 한 종류의 입력에 대한 관측값이다. 입력 분포·크기와 실행 환경이 바뀌면 상대 순위도 달라질 수 있다. `make report`로 다시 측정할 수 있다.",
            "",
            "실험을 다시 실행하려면 저장소 루트에서 `make report`를 실행한다.",
            "",
        ]
    )
    report_path = ROOT / "report.md"
    report_path.write_text("\n".join(report), encoding="utf-8")
    print(f"Wrote {report_path}")


if __name__ == "__main__":
    main()
