#include "sort.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { ARRAY_SIZE = 10000, WARMUPS = 1, REPEATS = 7, SORT_COUNT = 3 };

typedef struct {
    const char *name;
    SortFunction sort;
    const char *complexity;
    const char *stability;
    double median_ms;
    SortStats stats;
} SortResult;

static SortResult results[SORT_COUNT] = {
    {"Insertion", insertionSort, "O(n) / O(n^2) / O(n^2)", "안정", 0.0, {0}},
    {"Quick", quickSort, "O(n log n) / O(n log n) / O(n^2)", "불안정", 0.0, {0}},
    {"Merge", mergeSort, "O(n log n) / O(n log n) / O(n log n)", "안정", 0.0, {0}},
};

static int compareInts(const void *left, const void *right) {
    int left_value = *(const int *)left;
    int right_value = *(const int *)right;
    return (left_value > right_value) - (left_value < right_value);
}

static void makeInput(int values[], int count) {
    uint32_t state = 20260928u;
    for (int index = 0; index < count; index++) {
        state = state * 1664525u + 1013904223u;
        values[index] = (int)(state % 1000000u);
    }
}

static int matchesExpected(const int values[], const int expected[]) {
    return memcmp(values, expected, sizeof(int) * ARRAY_SIZE) == 0;
}

static void sortSamples(double samples[REPEATS]) {
    for (int index = 0; index < REPEATS; index++) {
        for (int next = index + 1; next < REPEATS; next++) {
            if (samples[next] < samples[index]) {
                double temporary = samples[index];
                samples[index] = samples[next];
                samples[next] = temporary;
            }
        }
    }
}

static size_t roundedAxisLimit(double value, size_t step) {
    size_t limit = (size_t)value;
    if ((double)limit < value) {
        limit++;
    }
    if (limit == 0) {
        return step;
    }
    size_t remainder = limit % step;
    return remainder == 0 ? limit : limit + step - remainder;
}

static int measureTime(SortResult *result, const int source[],
                       const int expected[]) {
    int values[ARRAY_SIZE];
    double samples[REPEATS];

    for (int warmup = 0; warmup < WARMUPS; warmup++) {
        memcpy(values, source, sizeof(values));
        result->sort(values, ARRAY_SIZE, NULL);
        if (!matchesExpected(values, expected)) {
            return 0;
        }
    }

    for (int repeat = 0; repeat < REPEATS; repeat++) {
        memcpy(values, source, sizeof(values));
        clock_t start = clock();
        result->sort(values, ARRAY_SIZE, NULL);
        clock_t end = clock();
        if (end == (clock_t)-1 || start == (clock_t)-1 ||
            !matchesExpected(values, expected)) {
            return 0;
        }
        samples[repeat] = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
    }

    sortSamples(samples);
    result->median_ms = samples[REPEATS / 2];
    return 1;
}

static void writeFileComposition(FILE *report) {
    fputs("### 파일 구성\n\n"
          "```mermaid\n"
          "flowchart TB\n"
          "    make[\"Makefile\"]\n"
          "    report[\"report.md\"]\n"
          "    subgraph src[\"src/\"]\n"
          "        header[\"sort.h\\nC 공통 인터페이스\"]\n"
          "        insertion[\"insertionSort.c\"]\n"
          "        quick[\"quickSort.c\"]\n"
          "        merge[\"mergesort.c\"]\n"
          "        main[\"main.c\"]\n"
          "    end\n"
          "    subgraph tools[\"tools/\"]\n"
          "        benchmark[\"benchmark.c\"]\n"
          "    end\n"
          "    subgraph tests[\"tests/\"]\n"
          "        test[\"test_sort.c\"]\n"
          "    end\n"
          "    insertion --> header\n"
          "    quick --> header\n"
          "    merge --> header\n"
          "    main --> header\n"
          "    test --> header\n"
          "    test --> insertion\n"
          "    test --> quick\n"
          "    test --> merge\n"
          "    benchmark --> header\n"
          "    benchmark --> insertion\n"
          "    benchmark --> quick\n"
          "    benchmark --> merge\n"
          "    benchmark --> report\n"
          "    make --> main\n"
          "    make --> test\n"
          "    make --> benchmark\n"
          "```\n\n"
          "| 파일 | 기능 |\n"
          "| --- | --- |\n"
          "| `Makefile` | C 프로그램 빌드·실행·테스트·보고서 생성 |\n"
          "| `report.md` | 정렬 알고리즘 설명과 C 실험 결과 |\n"
          "| `src/sort.h` | C 정렬 함수와 통계 구조체의 공통 인터페이스 |\n"
          "| `src/insertionSort.c` | C 삽입 정렬 구현 |\n"
          "| `src/quickSort.c` | C 퀵 정렬 구현 |\n"
          "| `src/mergesort.c` | C 병합 정렬 구현 |\n"
          "| `src/main.c` | 정렬 실행 예제와 통계 출력 |\n"
          "| `tools/benchmark.c` | C 정렬 성능 측정 및 `report.md` 생성 |\n"
          "| `tests/test_sort.c` | 표준 C로 정렬 결과와 통계 검증 |\n\n",
          report);
}

static void writeReport(FILE *report) {
    const SortResult *fastest = &results[0];
    const SortResult *least_memory = &results[0];
    double maximum_time_ms = 0.0;
    size_t maximum_memory_bytes = 0;

    for (int index = 0; index < SORT_COUNT; index++) {
        if (results[index].median_ms < fastest->median_ms) {
            fastest = &results[index];
        }
        if (results[index].stats.auxiliary_bytes <
            least_memory->stats.auxiliary_bytes) {
            least_memory = &results[index];
        }
        if (results[index].median_ms > maximum_time_ms) {
            maximum_time_ms = results[index].median_ms;
        }
        if (results[index].stats.auxiliary_bytes > maximum_memory_bytes) {
            maximum_memory_bytes = results[index].stats.auxiliary_bytes;
        }
    }
    size_t time_axis_max = roundedAxisLimit(maximum_time_ms, 50);
    size_t memory_axis_max = roundedAxisLimit((double)maximum_memory_bytes,
                                              10000);

    fputs("# 정렬 알고리즘 성능 비교\n\n"
          "## 차례\n\n"
          "1. [코드 보고서](#1-코드-보고서)\n"
          "2. [알고리즘 상세](#2-알고리즘-상세)\n"
          "3. [알고리즘 실험](#3-알고리즘-실험)\n\n"
          "## 1. 코드 보고서\n\n"
          "### 설계 선택과 근거\n\n"
          "삽입·퀵·병합 정렬을 C17로 구현해 같은 입력과 측정 조건에서 "
          "시간복잡도·비교 및 이동 횟수·보조 공간·재귀 깊이를 비교한다. "
          "세 함수는 공통 인터페이스로 정수 배열을 제자리 오름차순 정렬한다. "
          "외부 라이브러리 없이 표준 C 라이브러리만 사용해 실습 환경에서 "
          "바로 빌드하고 실행할 수 있게 했다.\n\n"
          "퀵 정렬은 가운데 원소를 피벗으로 삼아 작은 값·같은 값·큰 값으로 "
          "세 구간을 나누고, 작은 쪽을 먼저 재귀 호출해 스택 사용을 제한한다. "
          "병합 정렬은 임시 배열을 사용해 안정성을 지키며 O(n log n) 시간을 "
          "보장한다.\n\n",
          report);
    writeFileComposition(report);
    fputs("### 검증 방법\n\n"
          "`make test`로 섞인 배열, 정렬된 배열, 역순, 중복 값, 원소 하나, "
          "빈 배열의 결과와 통계 기록을 확인한다. 벤치마크는 측정 결과를 C "
          "표준 라이브러리 `qsort` 결과와 대조해 정렬 정확도를 확인한다. "
          "안정성은 동일 키 처리 순서를 보존하는지 구현의 동작 특성으로 "
          "분류한다.\n\n"
          "## 2. 알고리즘 상세\n\n"
          "### 삽입 정렬 (Insertion sort)\n\n"
          "왼쪽의 정렬된 구간을 유지하면서 다음 원소를 꺼내, 그보다 큰 원소를 "
          "오른쪽으로 옮긴 뒤 빈자리에 삽입한다. 거의 정렬됐거나 원소 수가 "
          "적으면 빠르지만, 역순에 가까운 입력에서는 이동이 많아진다. 최선은 "
          "O(n), 평균·최악은 O(n²)이며 제자리에서 안정적으로 정렬한다.\n\n"
          "```mermaid\n"
          "flowchart LR\n"
          "    A[\"입력: 5, 2, 4, 1\"] --> B[\"정렬 구간: 5\"]\n"
          "    B --> C[\"2를 앞에 삽입: 2, 5 | 4, 1\"]\n"
          "    C --> D[\"4를 알맞은 곳에 삽입: 2, 4, 5 | 1\"]\n"
          "    D --> E[\"1을 맨 앞에 삽입: 1, 2, 4, 5\"]\n"
          "```\n\n"
          "### 퀵 정렬 (Quick sort)\n\n"
          "피벗을 기준으로 작은 값·같은 값·큰 값의 세 구간으로 분할한 뒤, "
          "작은 값과 큰 값 구간을 재귀적으로 정렬한다. 평균은 O(n log n)이지만 "
          "분할이 한쪽으로 치우치면 최악 O(n²)이다. 가운데 원소를 피벗으로 "
          "사용하는 이 구현은 제자리 분할을 하지만 안정성은 보장하지 않는다.\n\n"
          "```mermaid\n"
          "flowchart LR\n"
          "    A[\"7, 2, 5, 2, 9, 1\"] --> B[\"피벗 5로 분할\"]\n"
          "    B --> C[\"작은 값: 1, 2, 2\"]\n"
          "    B --> D[\"같은 값: 5\"]\n"
          "    B --> E[\"큰 값: 7, 9\"]\n"
          "    C --> F[\"정렬 결과: 1, 2, 2, 5, 7, 9\"]\n"
          "    D --> F\n"
          "    E --> F\n"
          "```\n\n"
          "### 병합 정렬 (Merge sort)\n\n"
          "배열을 반으로 나눠 원소 하나까지 분해한 다음, 정렬된 두 구간을 "
          "작은 값부터 골라 합친다. 입력 상태와 무관하게 O(n log n) 시간이 "
          "들며 임시 버퍼로 O(n) 추가 공간을 사용한다. 같은 값에서는 왼쪽 "
          "원소를 먼저 선택하므로 안정 정렬이다.\n\n"
          "```mermaid\n"
          "flowchart TD\n"
          "    A[\"7, 2, 5, 1\"] --> B[\"7, 2\"]\n"
          "    A --> C[\"5, 1\"]\n"
          "    B --> D[\"7\"]\n"
          "    B --> E[\"2\"]\n"
          "    C --> F[\"5\"]\n"
          "    C --> G[\"1\"]\n"
          "    D --> H[\"2, 7\"]\n"
          "    E --> H\n"
          "    F --> I[\"1, 5\"]\n"
          "    G --> I\n"
          "    H --> J[\"1, 2, 5, 7\"]\n"
          "    I --> J\n"
          "```\n\n"
          "## 3. 알고리즘 실험\n\n"
          "같은 C 구현을 같은 고정 입력과 측정 절차로 비교했다. 실행 시간, "
          "연산 횟수, 보조 공간, 재귀 깊이를 표와 그래프로 나타낸다.\n\n"
          "### 실험 설계\n\n",
          report);

    fprintf(report,
            "C17로 빌드한 C 구현을 측정했다. 고정 시드 `20260928`로 "
            "0 이상 1,000,000 미만의 정수 %d개를 만들고, 세 정렬에 같은 "
            "원본을 제공했다. 각 실행 직전에 복사하므로 모든 측정은 같은 "
            "입력에서 시작한다.\n\n"
            "준비 실행 %d회를 버린 뒤 `clock()`으로 실행 시간을 %d회 측정하고 "
            "중앙값을 사용했다. 입력 복사는 타이머 시작 전에 했다. 비교·이동 "
            "횟수, 보조 공간, 재귀 깊이는 별도 1회 계측에서 얻었다. 보조 공간은 "
            "구현이 기록한 알고리즘 보조 메모리이며 입력과 호출 스택은 제외한다. "
            "안정성은 측정값이 아니라 동률 처리 동작으로 판별한다.\n\n"
            "```mermaid\n"
            "flowchart LR\n"
            "    A[\"고정 시드 20260928\"] --> B[\"동일한 정수 %d개 생성\"]\n"
            "    B --> C[\"각 정렬에 같은 입력 복사\"]\n"
            "    C --> D[\"준비 %d회 후 시간 %d회 측정\"]\n"
            "    D --> E[\"중앙 실행 시간\"]\n"
            "    C --> F[\"비교·이동·공간·재귀 깊이 계측\"]\n"
            "    C --> G[\"qsort 결과와 대조\"]\n"
            "    E --> H[\"표와 그래프로 report.md 작성\"]\n"
            "    F --> H\n"
            "    G --> H\n"
            "```\n\n"
            "### 측정 결과\n\n"
            "| 알고리즘 | 시간복잡도 (최선 / 평균 / 최악) | 중앙 시간 (ms) | 비교 횟수 | 이동 횟수 | 보조 공간 (bytes) | 최대 재귀 깊이 | 안정성 |\n"
            "| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |\n",
            ARRAY_SIZE, WARMUPS, REPEATS, ARRAY_SIZE, WARMUPS, REPEATS);

    for (int index = 0; index < SORT_COUNT; index++) {
        fprintf(report,
                "| %s sort | %s | %.3f | %zu | %zu | %zu | %u | %s |\n",
                results[index].name, results[index].complexity,
                results[index].median_ms, results[index].stats.comparisons,
                results[index].stats.moves,
                results[index].stats.auxiliary_bytes,
                results[index].stats.max_recursion_depth,
                results[index].stability);
    }

    fputs("\n### 실행 시간 그래프\n\n"
          "```mermaid\n"
          "xychart-beta\n"
          "    title \"중앙 실행 시간 (ms)\"\n"
          "    x-axis [\"Insertion\", \"Quick\", \"Merge\"]\n",
          report);
    fprintf(report,
            "    y-axis \"ms\" 0 --> %zu\n    bar [%.3f, %.3f, %.3f]\n```\n\n",
            time_axis_max, results[0].median_ms, results[1].median_ms,
            results[2].median_ms);

    fputs("### 보조 공간 그래프\n\n"
          "```mermaid\n"
          "xychart-beta\n"
          "    title \"알고리즘 보조 공간 (bytes)\"\n"
          "    x-axis [\"Insertion\", \"Quick\", \"Merge\"]\n",
          report);
    fprintf(report,
            "    y-axis \"bytes\" 0 --> %zu\n    bar [%zu, %zu, %zu]\n```\n\n",
            memory_axis_max, results[0].stats.auxiliary_bytes,
            results[1].stats.auxiliary_bytes,
            results[2].stats.auxiliary_bytes);

    fprintf(report,
            "실행 시간은 이 환경에서 가장 짧은 중앙값이 **%s sort**였고, "
            "기록된 보조 공간이 가장 적은 알고리즘은 **%s sort**였다. "
            "삽입 정렬은 무작위 입력에서 이차 시간 증가가 나타나며, 병합 정렬은 "
            "선형 보조 공간을 사용한다. 이 결과는 한 입력 크기와 실행 환경의 "
            "관측값이며, 입력 분포와 환경에 따라 달라질 수 있다.\n\n"
            "실험을 다시 실행하려면 저장소 루트에서 `make report`를 실행한다.\n",
            fastest->name, least_memory->name);
}

int main(void) {
    int source[ARRAY_SIZE];
    int expected[ARRAY_SIZE];
    makeInput(source, ARRAY_SIZE);
    memcpy(expected, source, sizeof(source));
    qsort(expected, ARRAY_SIZE, sizeof(expected[0]), compareInts);

    for (int index = 0; index < SORT_COUNT; index++) {
        int values[ARRAY_SIZE];
        memcpy(values, source, sizeof(values));
        results[index].sort(values, ARRAY_SIZE, &results[index].stats);
        if (!matchesExpected(values, expected) ||
            !measureTime(&results[index], source, expected)) {
            fprintf(stderr, "%s sort failed benchmark validation\n",
                    results[index].name);
            return 1;
        }
    }

    FILE *report = fopen("report.md", "w");
    if (report == NULL) {
        perror("report.md");
        return 1;
    }
    writeReport(report);
    if (fclose(report) != 0) {
        perror("report.md");
        return 1;
    }

    printf("Wrote report.md with C-only measurements (n=%d, median of %d runs)\n",
           ARRAY_SIZE, REPEATS);
    return 0;
}
