#include "sort.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum {
    ARRAY_SIZE = 10000,
    WARMUPS = 1,
    REPEATS = 7,
    SORT_COUNT = 3,
    SORTEDNESS_LEVEL_COUNT = 7,
    SMALL_ARRAY_MAX = 128,
    SMALL_BATCH_TARGET = 4096,
    SMALL_BATCH_MIN = 32,
    CROSSOVER_STREAK = 5,
    CROSSOVER_COUNT = 3,
};

static const int sortedness_levels[SORTEDNESS_LEVEL_COUNT] = {
    0, 1, 5, 10, 25, 50, 100};
static const int displayed_small_sizes[] = {1, 2, 4, 8, 16, 32, 64, 128};
typedef struct {
    const char *name;
    SortFunction sort;
    const char *complexity;
    const char *stability;
    double median_ms;
    SortStats stats;
} SortResult;

typedef struct {
    int shuffled_percent;
    double median_ms[SORT_COUNT];
} SortednessResult;

static SortResult results[SORT_COUNT] = {
    {"Insertion", insertionSort, "O(n) / O(n^2) / O(n^2)", "안정", 0.0, {0}},
    {"Quick", quickSort, "O(n log n) / O(n log n) / O(n^2)", "불안정", 0.0, {0}},
    {"Tree", treeSort, "O(n) / O(n log n) / O(n^2)", "불안정", 0.0, {0}},
};

static SortednessResult sortedness_results[SORTEDNESS_LEVEL_COUNT];
static double small_array_times[SORT_COUNT][SMALL_ARRAY_MAX + 1];

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

static int matchesExpected(const int values[], const int expected[], int count) {
    return memcmp(values, expected, sizeof(int) * (size_t)count) == 0;
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

static int measureTimeForSize(SortFunction sort, const int source[],
                              const int expected[], int count,
                              double *median_ms) {
    int values[ARRAY_SIZE];
    double samples[REPEATS];
    if (count < 0 || count > ARRAY_SIZE) {
        return 0;
    }

    for (int warmup = 0; warmup < WARMUPS; warmup++) {
        memcpy(values, source, sizeof(int) * (size_t)count);
        sort(values, count, NULL);
        if (!matchesExpected(values, expected, count)) {
            return 0;
        }
    }

    for (int repeat = 0; repeat < REPEATS; repeat++) {
        memcpy(values, source, sizeof(int) * (size_t)count);
        clock_t start = clock();
        sort(values, count, NULL);
        clock_t end = clock();
        if (end == (clock_t)-1 || start == (clock_t)-1 ||
            !matchesExpected(values, expected, count)) {
            return 0;
        }
        samples[repeat] = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
    }

    sortSamples(samples);
    *median_ms = samples[REPEATS / 2];
    return 1;
}

static int measureTime(SortResult *result, const int source[],
                       const int expected[]) {
    return measureTimeForSize(result->sort, source, expected, ARRAY_SIZE,
                              &result->median_ms);
}

static void makeSortednessInput(int values[], int count, int shuffled_percent) {
    int shuffled_count = count * shuffled_percent / 100;
    uint32_t state = 20260928u + (uint32_t)shuffled_percent * 1013904223u;
    for (int index = 0; index < count; index++) {
        values[index] = index;
    }
    for (int index = shuffled_count - 1; index > 0; index--) {
        state = state * 1664525u + 1013904223u;
        int other = (int)(state % (uint32_t)(index + 1));
        int temporary = values[index];
        values[index] = values[other];
        values[other] = temporary;
    }
}

static int runSortednessMeasurements(void) {
    int source[ARRAY_SIZE];
    int expected[ARRAY_SIZE];
    for (int index = 0; index < ARRAY_SIZE; index++) {
        expected[index] = index;
    }

    for (int level = 0; level < SORTEDNESS_LEVEL_COUNT; level++) {
        int shuffled_percent = sortedness_levels[level];
        sortedness_results[level].shuffled_percent = shuffled_percent;
        makeSortednessInput(source, ARRAY_SIZE, shuffled_percent);
        for (int sort_index = 0; sort_index < SORT_COUNT; sort_index++) {
            if (!measureTimeForSize(results[sort_index].sort, source, expected,
                                    ARRAY_SIZE,
                                    &sortedness_results[level]
                                         .median_ms[sort_index])) {
                return 0;
            }
        }
    }
    return 1;
}

static int measureSmallArrayTime(SortFunction sort, int count,
                                 double *median_microseconds) {
    int source[SMALL_ARRAY_MAX];
    int expected[SMALL_ARRAY_MAX];
    makeInput(source, count);
    memcpy(expected, source, sizeof(int) * (size_t)count);
    qsort(expected, (size_t)count, sizeof(expected[0]), compareInts);

    size_t batch_count = SMALL_BATCH_TARGET / (size_t)count;
    if (batch_count < SMALL_BATCH_MIN) {
        batch_count = SMALL_BATCH_MIN;
    }
    size_t batch_items = batch_count * (size_t)count;
    int *batch = malloc(batch_items * sizeof(*batch));
    if (batch == NULL) {
        return 0;
    }

    for (size_t index = 0; index < batch_count; index++) {
        memcpy(batch + index * (size_t)count, source,
               sizeof(int) * (size_t)count);
        sort(batch + index * (size_t)count, count, NULL);
        if (!matchesExpected(batch + index * (size_t)count, expected, count)) {
            free(batch);
            return 0;
        }
    }

    double samples[REPEATS];
    for (int repeat = 0; repeat < REPEATS; repeat++) {
        for (size_t index = 0; index < batch_count; index++) {
            memcpy(batch + index * (size_t)count, source,
                   sizeof(int) * (size_t)count);
        }
        clock_t start = clock();
        if (start == (clock_t)-1) {
            free(batch);
            return 0;
        }
        for (size_t index = 0; index < batch_count; index++) {
            sort(batch + index * (size_t)count, count, NULL);
        }
        clock_t end = clock();
        if (end == (clock_t)-1 || end < start) {
            free(batch);
            return 0;
        }
        for (size_t index = 0; index < batch_count; index++) {
            if (!matchesExpected(batch + index * (size_t)count, expected,
                                 count)) {
                free(batch);
                return 0;
            }
        }
        samples[repeat] = 1000000.0 * (double)(end - start) /
                          CLOCKS_PER_SEC / (double)batch_count;
    }

    sortSamples(samples);
    *median_microseconds = samples[REPEATS / 2];
    free(batch);
    return 1;
}

static int runSmallArrayMeasurements(void) {
    for (int size = 1; size <= SMALL_ARRAY_MAX; size++) {
        for (int sort_index = 0; sort_index < SORT_COUNT; sort_index++) {
            if (!measureSmallArrayTime(results[sort_index].sort, size,
                                       &small_array_times[sort_index][size])) {
                return 0;
            }
        }
    }

    return 1;
}

static double smoothedSmallArrayTime(int sort_index, int size) {
    int first_size = size > 2 ? size - 2 : 1;
    int last_size = size + 2 < SMALL_ARRAY_MAX ? size + 2 : SMALL_ARRAY_MAX;
    double window[5];
    int count = 0;

    for (int current_size = first_size; current_size <= last_size;
         current_size++) {
        window[count++] = small_array_times[sort_index][current_size];
    }
    for (int index = 1; index < count; index++) {
        double value = window[index];
        int position = index;
        while (position > 0 && window[position - 1] > value) {
            window[position] = window[position - 1];
            position--;
        }
        window[position] = value;
    }
    return window[count / 2];
}

static int findCrossoverSize(int initial_winner, int final_winner) {
    if (initial_winner == final_winner) {
        return 0;
    }
    int incumbent = initial_winner;
    int challenger = final_winner;
    for (int size = 2; size <= SMALL_ARRAY_MAX - CROSSOVER_STREAK + 1; size++) {
        int challenger_is_faster = 1;
        for (int offset = 0; offset < CROSSOVER_STREAK; offset++) {
            if (smoothedSmallArrayTime(challenger, size + offset) >=
                smoothedSmallArrayTime(incumbent, size + offset)) {
                challenger_is_faster = 0;
                break;
            }
        }
        if (challenger_is_faster) {
            return size;
        }
    }
    return 0;
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
          "        tree[\"treeSort.c\"]\n"
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
          "    tree --> header\n"
          "    main --> header\n"
          "    test --> header\n"
          "    test --> insertion\n"
          "    test --> quick\n"
          "    test --> tree\n"
          "    benchmark --> header\n"
          "    benchmark --> insertion\n"
          "    benchmark --> quick\n"
          "    benchmark --> tree\n"
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
          "| `src/treeSort.c` | 이진 탐색 트리를 만들고 중위 순회로 정렬하는 tree sort |\n"
          "| `src/main.c` | 정렬 실행 예제와 통계 출력 |\n"
          "| `tools/benchmark.c` | C 정렬 성능 측정 및 `report.md` 생성 |\n"
          "| `tests/test_sort.c` | 표준 C로 정렬 결과와 통계 검증 |\n\n",
          report);
}

static void writeSeriesPalette(FILE *report) {
    fputs("---\n"
          "config:\n"
          "  xyChart:\n"
          "    plotColorPalette: \"#0072B2, #D55E00, #009E73\"\n"
          "---\n",
          report);
}

static void writeAlgorithmLegend(FILE *report) {
    fputs("선 색상은 파랑=Insertion sort, 주황=Quick sort, "
          "초록=Tree sort 순서다.\n\n",
          report);
}

static void writeSortednessResults(FILE *report) {
    double maximum_time_ms = 0.0;
    for (int level = 0; level < SORTEDNESS_LEVEL_COUNT; level++) {
        for (int sort_index = 0; sort_index < SORT_COUNT; sort_index++) {
            if (sortedness_results[level].median_ms[sort_index] >
                maximum_time_ms) {
                maximum_time_ms =
                    sortedness_results[level].median_ms[sort_index];
            }
        }
    }
    size_t axis_max = roundedAxisLimit(maximum_time_ms, 10);

    fputs("### 입력 정렬 정도에 따른 성능\n\n"
          "입력 크기는 10,000개로 고정했다. 정렬된 입력의 앞부분에서 지정한 "
          "비율만 Fisher–Yates 방식으로 섞고, 나머지 뒷부분은 정렬 상태로 "
          "두었다. 비율이 높을수록 섞인 구간이 커진다. 표의 시간은 7회 측정한 "
          "중앙값이다.\n\n"
          "| 섞은 앞부분 (%) | Insertion (ms) | Quick (ms) | Tree (ms) |\n"
          "| ---: | ---: | ---: | ---: |\n",
          report);
    for (int level = 0; level < SORTEDNESS_LEVEL_COUNT; level++) {
        fprintf(report, "| %d | %.3f | %.3f | %.3f |\n",
                sortedness_results[level].shuffled_percent,
                sortedness_results[level].median_ms[0],
                sortedness_results[level].median_ms[1],
                sortedness_results[level].median_ms[2]);
    }

    fputs("\n```mermaid\n", report);
    writeSeriesPalette(report);
    fputs("xychart-beta\n"
          "    title \"섞은 구간 비율별 실행 시간 (ms)\"\n"
          "    x-axis [",
          report);
    for (int level = 0; level < SORTEDNESS_LEVEL_COUNT; level++) {
        fprintf(report, "%s\"%d%%\"",
                level == 0 ? "" : ", ",
                sortedness_results[level].shuffled_percent);
    }
    fprintf(report, "]\n    y-axis \"ms\" 0 --> %zu\n", axis_max);
    for (int sort_index = 0; sort_index < SORT_COUNT; sort_index++) {
        fputs("    line [", report);
        for (int level = 0; level < SORTEDNESS_LEVEL_COUNT; level++) {
            fprintf(report, "%s%.3f", level == 0 ? "" : ", ",
                    sortedness_results[level].median_ms[sort_index]);
        }
        fputs("]\n", report);
    }
    fputs("```\n\n", report);
    writeAlgorithmLegend(report);
}

static void writeSmallArrayResults(FILE *report) {
    double maximum_time_microseconds = 0.0;
    for (int sort_index = 0; sort_index < SORT_COUNT; sort_index++) {
        for (int size = 1; size <= SMALL_ARRAY_MAX; size++) {
            if (small_array_times[sort_index][size] >
                maximum_time_microseconds) {
                maximum_time_microseconds = small_array_times[sort_index][size];
            }
        }
    }
    double axis_max = maximum_time_microseconds * 1.1;
    if (axis_max == 0.0) {
        axis_max = 1.0;
    }

    fputs("### 작은 배열의 성능과 교차점\n\n"
          "무작위 정수 입력의 크기를 1개부터 128개까지 1개씩 늘려 측정했다. "
          "표와 그래프는 대표 크기를 보여 주며, 교차점 탐색은 모든 크기 측정값을 "
          "사용한다. 시간은 배열 한 개당 중앙값(µs)이다.\n\n"
          "| 원소 수 | Insertion (µs) | Quick (µs) | Tree (µs) |\n"
          "| ---: | ---: | ---: | ---: |\n",
          report);
    for (size_t index = 0;
         index < sizeof(displayed_small_sizes) / sizeof(displayed_small_sizes[0]);
         index++) {
        int size = displayed_small_sizes[index];
        fprintf(report, "| %d | %.4f | %.4f | %.4f |\n", size,
                small_array_times[0][size], small_array_times[1][size],
                small_array_times[2][size]);
    }

    fputs("\n```mermaid\n", report);
    writeSeriesPalette(report);
    fputs("xychart-beta\n"
          "    title \"배열 크기별 실행 시간 (µs)\"\n"
          "    x-axis [",
          report);
    for (size_t index = 0;
         index < sizeof(displayed_small_sizes) / sizeof(displayed_small_sizes[0]);
         index++) {
        fprintf(report, "%s\"%d\"",
                index == 0 ? "" : ", ", displayed_small_sizes[index]);
    }
    fprintf(report, "]\n    y-axis \"µs\" 0 --> %.4f\n", axis_max);
    for (int sort_index = 0; sort_index < SORT_COUNT; sort_index++) {
        fputs("    line [", report);
        for (size_t index = 0;
             index < sizeof(displayed_small_sizes) /
                        sizeof(displayed_small_sizes[0]);
             index++) {
            int size = displayed_small_sizes[index];
            fprintf(report, "%s%.4f", index == 0 ? "" : ", ",
                    small_array_times[sort_index][size]);
        }
        fputs("]\n", report);
    }
    fputs("```\n\n", report);
    writeAlgorithmLegend(report);
    fputs("#### 소형 배열 교차점\n\n"
          "초기 우세는 2–6개 구간의 평활 시간 중앙값으로, 큰 배열 쪽 우세는 "
          "126–128개 구간의 평활 시간 중앙값으로 판별한다. 두 구간의 승자가 "
          "다르면 큰 배열 쪽 알고리즘이 5개 연속 크기에서 처음 더 빨라지는 "
          "크기를 교차점으로 기록한다. 각 크기의 평활 시간은 인접한 최대 5개 "
          "크기 측정값의 중앙값이다.\n\n"
          "| 알고리즘 쌍 | 측정된 교차점 |\n"
          "| --- | --- |\n",
          report);
    int pair_indices[CROSSOVER_COUNT][2] = {{0, 1}, {0, 2}, {1, 2}};
    for (int pair = 0; pair < CROSSOVER_COUNT; pair++) {
        int left_index = pair_indices[pair][0];
        int right_index = pair_indices[pair][1];
        int initial_winner =
            smoothedSmallArrayTime(left_index, 4) <=
                    smoothedSmallArrayTime(right_index, 4)
                ? left_index
                : right_index;
        int final_winner =
            smoothedSmallArrayTime(left_index, SMALL_ARRAY_MAX) <=
                    smoothedSmallArrayTime(right_index, SMALL_ARRAY_MAX)
                ? left_index
                : right_index;
        int crossover_size =
            findCrossoverSize(initial_winner, final_winner);

        if (crossover_size > 0) {
            fprintf(report,
                    "| %s ↔ %s | %d개부터 %s sort가 %s sort보다 5개 연속 크기에서 빠름 |\n",
                    results[left_index].name, results[right_index].name,
                    crossover_size, results[final_winner].name,
                    results[initial_winner].name);
        } else if (initial_winner == final_winner) {
            fprintf(report,
                    "| %s ↔ %s | 1–%d개에서 지속 교차 없음 (%s sort가 양 끝 구간에서 우세) |\n",
                    results[left_index].name, results[right_index].name,
                    SMALL_ARRAY_MAX, results[initial_winner].name);
        } else {
            fprintf(report,
                    "| %s ↔ %s | 끝 구간 우세 전환은 있으나 5개 연속 교차점은 확인되지 않음 |\n",
                    results[left_index].name, results[right_index].name);
        }
    }
    fputs("\n소형 입력은 측정 시간의 해상도 영향을 줄이기 위해 같은 크기의 "
          "배열 여러 개를 한 묶음으로 정렬했다. 복사와 결과 확인은 시간 측정 "
          "구간에서 제외했다.\n\n",
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
    size_t time_axis_max = roundedAxisLimit(maximum_time_ms, 10);
    size_t memory_axis_max =
        roundedAxisLimit((double)maximum_memory_bytes, 5);

    fputs("# 정렬 알고리즘 성능 비교\n\n"
          "## 차례\n\n"
          "1. [코드 보고서](#1-코드-보고서)\n"
          "2. [알고리즘 상세](#2-알고리즘-상세)\n"
          "3. [알고리즘 실험](#3-알고리즘-실험)\n\n"
          "## 1. 코드 보고서\n\n"
          "### 1.1 설계 선택과 근거\n\n"
          "삽입·퀵·tree sort를 C17로 구현해 같은 입력과 측정 조건에서 "
          "시간복잡도·비교 및 이동 횟수·보조 공간·재귀 깊이를 비교한다. "
          "세 함수는 공통 인터페이스로 정수 배열을 제자리 오름차순 정렬한다. "
          "외부 라이브러리 없이 표준 C 라이브러리만 사용해 실습 환경에서 "
          "바로 빌드하고 실행할 수 있게 했다.\n\n"
          "### 1.2 인터페이스 설계\n\n"
          "| 선택한 설계 | 선택 이유 |\n"
          "| --- | --- |\n"
          "| `typedef void (*SortFunction)(int a[], int n, SortStats *stats)` | 세 정렬을 같은 함수 포인터 형식으로 호출해 실행 예제·테스트·벤치마크에서 알고리즘별 어댑터가 필요 없게 한다. |\n"
          "| 입력 배열을 오름차순으로 제자리 정렬하고 반환값은 두지 않음 | 별도 결과 배열을 만들지 않아도 되고, 출력 계약을 모든 구현에서 동일하게 유지한다. |\n"
          "| `SortStats *`를 선택 인자로 전달하고 `NULL`을 허용 | 통계가 필요한 실행에서는 비교·이동·보조 공간·재귀 깊이를 기록하고, 시간 측정에서는 계측 비용을 끌 수 있다. |\n"
          "| `SortFunction` 함수 포인터로 공통 타입 정의 | 정렬 함수들을 배열에 담아 같은 입력·검사·측정 절차로 반복 실행할 수 있다. |\n"
          "| 길이 `n`을 호출자가 전달 | 포인터와 배열 길이를 명시해 빈 배열도 `n = 0`으로 처리하고, 구현이 길이를 추정하지 않게 한다. |\n\n"
          "퀵 정렬은 가운데 원소를 피벗으로 삼아 작은 값·같은 값·큰 값으로 "
          "세 구간을 나누고, 작은 쪽을 먼저 재귀 호출해 스택 사용을 제한한다. "
          "Tree sort는 값별 노드를 이진 탐색 트리로 연결하고 중위 순회로 "
          "오름차순 결과를 만든다. 중복은 노드의 개수로 묶어 저장하며, 균형이 "
          "맞으면 O(n log n), 입력 순서대로 한쪽으로 기울면 O(n²) 시간이 걸린다.\n\n",
          report);
    writeFileComposition(report);
        fputs("### 1.4 검증 방법\n\n"
            "1. **빌드·링크**: `make test`가 `tests/test_sort.c`와 세 정렬 구현을 빌드한 뒤 테스트를 실행한다.\n"
            "2. **정렬 결과**: 섞임·정렬됨·역순·중복·원소 하나·빈 배열을 세 알고리즘에 적용해 총 18개 결과를 비교한다.\n"
            "3. **통계 기록**: 세 정렬에 `{3, 1, 2}`를 넣어 정렬 결과와 비교·이동·보조 공간 카운터 기록 여부를 확인한다.\n"
            "4. **벤치마크 정답**: 10,000개 입력을 `qsort` 결과와 대조하며, 불일치하면 보고서 생성을 중단한다.\n"
            "5. **시간 측정**: 준비 실행 후 복사를 제외하고 7회 재며, 중앙값을 사용한다. 연산·공간 통계는 별도로 잰다.\n"
            "6. **안정성**: 레코드 순서를 직접 시험하지 않고, 같은 값의 순서를 보존하는지 구현의 동률 처리로 판별한다.\n"
            "7. **변이 테스트**: 실제 코드는 그대로 두고 `/tmp` 복사본에 결함을 넣어 테스트가 검출하는지 확인했다.\n\n"
            "| 망가뜨린 곳 | 결과 |\n"
            "| --- | --- |\n"
            "| Tree sort에서 중복 원소 개수 증가 제거 | 중복 입력 검사 실패 (21개 중 1개, 종료 코드 1) |\n"
            "| Tree sort에서 오른쪽 하위 트리 순회 생략 | 통계·섞인 배열·중복 입력 검사 실패 (21개 중 3개, 종료 코드 1) |\n\n"
          "## 2. 알고리즘 상세\n\n"
          "### 삽입 정렬 (Insertion sort)\n\n"
          "왼쪽의 정렬된 구간을 유지하면서 다음 원소를 꺼내, 그보다 큰 원소를 "
          "오른쪽으로 옮긴 뒤 빈자리에 삽입한다. 거의 정렬됐거나 원소 수가 "
          "적으면 빠르지만, 역순에 가까운 입력에서는 이동이 많아진다. 최선은 "
          "O(n), 평균·최악은 O(n²)이며 제자리에서 안정적으로 정렬한다.\n\n"
          "핵심은 현재 값을 보관하고, 그보다 큰 앞쪽 원소만 오른쪽으로 "
          "밀어 빈자리에 삽입하는 것이다.\n\n"
          "```c\n"
          "int value = a[i];\n"
          "int j = i - 1;\n"
          "while (j >= 0 && a[j] > value) {\n"
          "    a[j + 1] = a[j];\n"
          "    j--;\n"
          "}\n"
          "a[j + 1] = value;\n"
          "```\n\n"
          "`>` 조건은 같은 값은 지나치지 않으므로 입력 순서를 유지한다.\n\n"
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
          "`less`, `scan`, `greater` 경계로 작은 값·미확인 값·큰 값을 나누며 "
          "한 번의 순회로 피벗 기준 세 구간을 만든다.\n\n"
          "```c\n"
          "while (scan <= greater) {\n"
          "    if (a[scan] < pivot) {\n"
          "        int tmp = a[less];\n"
          "        a[less++] = a[scan];\n"
          "        a[scan++] = tmp;\n"
          "    } else if (a[scan] > pivot) {\n"
          "        int tmp = a[greater];\n"
          "        a[greater--] = a[scan];\n"
          "        a[scan] = tmp;\n"
          "    } else {\n"
          "        scan++;\n"
          "    }\n"
          "}\n"
          "```\n\n"
          "작은 값은 왼쪽으로, 큰 값은 오른쪽으로 보낸다. 큰 값과 바꿔 온 "
          "원소는 아직 확인하지 않았으므로 `scan`을 그대로 둔다.\n\n"
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
          "### 트리 정렬 (Tree sort)\n\n"
          "입력 원소를 이진 탐색 트리에 삽입한 뒤, 중위 순회(left → node → right) "
          "결과를 배열에 다시 써 오름차순으로 만든다. 이 구현은 반복형 삽입과 "
          "반복형 순회를 사용해 트리가 한쪽으로 기울어도 호출 스택이 깊어지지 않게 "
          "한다. 같은 정수는 새 노드를 만들지 않고 해당 노드의 `occurrences`를 "
          "증가시킨다.\n\n"
          "이진 탐색 트리는 각 노드가 값과 왼쪽·오른쪽 자식을 갖는 구조다. 왼쪽 "
          "하위 트리에는 노드보다 작은 값, 오른쪽 하위 트리에는 큰 값을 둔다. "
          "같은 값은 새 노드 대신 기존 노드의 `occurrences`에 센다.\n\n"
          "예를 들어 `7, 2, 9, 1, 5, 8, 5`를 차례로 삽입하면 다음 구조가 된다. "
          "두 번째 5는 새 노드를 만들지 않고 기존 5의 개수에 합친다.\n\n"
          "```mermaid\n"
          "flowchart TD\n"
          "    n7[\"7\"] -->|왼쪽| n2[\"2\"]\n"
          "    n7 -->|오른쪽| n9[\"9\"]\n"
          "    n2 -->|왼쪽| n1[\"1\"]\n"
          "    n2 -->|오른쪽| n5[\"5 (2개)\"]\n"
          "    n9 -->|왼쪽| n8[\"8\"]\n"
          "```\n\n"
          "#### 1단계: 이진 탐색 트리 구성\n\n"
          "각 값은 현재 노드보다 작으면 왼쪽, 크면 오른쪽으로 내려간다. 같은 값이면 "
          "개수만 늘린다. 노드를 만들 때까지 이 과정을 반복한다.\n\n"
          "```c\n"
          "while (*link != NULL) {\n"
          "    if (value < (*link)->value) link = &(*link)->left;\n"
          "    else if (value > (*link)->value) link = &(*link)->right;\n"
          "    else { (*link)->occurrences++; break; }\n"
          "}\n"
          "```\n\n"
          "노드 삽입 순서에 따라 트리 모양이 달라진다. 균형이 잘 맞으면 각 삽입이 "
          "짧은 경로를 따라가지만, 이미 정렬된 입력은 계속 같은 방향의 자식으로만 "
          "연결돼 긴 사슬이 될 수 있다.\n\n"
          "#### 2단계: 반복형 중위 순회\n\n"
          "스택에 왼쪽 경로를 저장했다가 가장 작은 노드부터 꺼낸다. 노드를 꺼낼 "
          "때 `occurrences`만큼 값을 출력 배열에 기록하고, 오른쪽 하위 트리로 "
          "이동한다.\n\n"
          "```c\n"
          "while (current != NULL || stack_size > 0) {\n"
          "    while (current != NULL) {\n"
          "        stack[stack_size++] = current;\n"
          "        current = current->left;\n"
          "    }\n"
          "    current = stack[--stack_size];\n"
          "    writeValue(current->value, current->occurrences);\n"
          "    current = current->right;\n"
          "}\n"
          "```\n\n"
          "`writeValue`는 동작을 보여 주기 위한 축약 표현이며, 실제 구현은 반복문으로 "
          "해당 값을 `occurrences`번 배열에 쓴다. 트리 구성이 끝날 때까지 원본 배열을 "
          "수정하지 않으므로 노드나 스택 할당에 실패하면 입력은 그대로 남는다.\n\n"
          "#### 복잡도·공간·안정성\n\n"
          "노드 삽입 비용은 트리 높이를 `h`라 할 때 O(nh), 중위 순회는 O(n)이다. "
          "균형 트리에서는 높이가 O(log n)이므로 평균적인 시간은 O(n log n)이지만, "
          "입력 순서가 오름차순 또는 내림차순이면 높이가 O(n)이 되어 최악 시간은 "
          "O(n²)이다. 노드와 순회 스택을 저장하므로 추가 공간은 O(n)이다. 중복을 "
          "개수로 합치고 원래 순서는 저장하지 않으므로 안정 정렬은 아니다.\n\n"
          "```mermaid\n"
          "flowchart LR\n"
          "    A[\"입력: 7, 2, 5, 2, 1\"] --> B[\"BST 삽입\"]\n"
          "    B --> C[\"중위 순회\"]\n"
          "    C --> D[\"출력: 1, 2, 2, 5, 7\"]\n"
          "```\n\n"
          "## 3. 알고리즘 실험\n\n"
          "같은 C 구현을 같은 고정 입력과 측정 절차로 비교했다. 실행 시간, "
          "연산 횟수, 보조 공간, 재귀 깊이를 표와 그래프로 나타낸다.\n\n"
            "### 3.1 실험 설계\n\n",
          report);

        fputs("모든 구현은 컨테이너의 `gcc -std=c17 -Wall -Wextra -O2` 환경에서 측정했다.\n\n"
            "| 무엇을 재는가 | 방법 | 주의사항 |\n"
            "| --- | --- | --- |\n"
            "| 기본 실행 시간 | 고정 시드 `20260928`로 만든 같은 정수 10,000개를 사용한다. 준비 1회 후 `clock()`으로 7회 재고 중앙값을 낸다. | 입력 복사와 정답 확인은 시간에서 제외한다. 한 컴파일러·환경에서 얻은 값이다. |\n"
            "| 정렬 정확도 | `qsort` 결과를 기준 배열로 삼아 계측·준비·시간 측정 실행 결과를 대조한다. | 확인 비용은 실행 시간에 포함하지 않으며, 불일치하면 보고서 생성을 중단한다. |\n"
            "| 비교·이동 횟수 | 별도 실행에서 `SortStats` 카운터를 읽는다. | 구현이 정한 연산 단위의 카운트이며 실제 CPU 명령 수는 아니다. |\n"
            "| 보조 메모리 | 각 구현이 `SortStats.auxiliary_bytes`에 기록한 값을 읽는다. | 입력 배열과 호출 스택은 제외한다. Tree sort의 노드와 순회 스택은 포함한다. |\n"
            "| 재귀 깊이 | 별도 계측 실행에서 `max_recursion_depth`를 읽는다. | 재귀 호출 깊이만 뜻한다. 반복형 Tree sort에서는 트리 높이를 나타내지 않는다. |\n"
            "| 입력 정렬 정도별 시간 | 10,000개 오름차순 값에서 앞부분의 0·1·5·10·25·50·100%%만 Fisher–Yates로 섞어 7회 측정한다. | 나머지 뒷부분은 정렬 상태다. 임의의 전체 배열 교란과는 다른 입력 모델이다. |\n"
            "| 작은 배열 시간·교차점 | 무작위 배열 크기 1~128을 모두 측정하고, 같은 크기의 여러 배열을 묶어 배열당 중앙 시간을 낸다. | 교차점은 인접 크기 최대 5개의 중앙값으로 평활화하고, 5개 연속 크기 우세를 기준으로 한다. |\n"
            "| 안정성 | 같은 키에서 원래 순서를 보존하는지 구현의 동률 처리로 판별한다. | 정수 전용 테스트라 꼬리표가 있는 레코드로 직접 측정하지 않는다. |\n\n"
            "#### 안정성 판별 상세\n\n"
            "안정 정렬은 정렬 키가 같은 항목들의 입력 순서를 정렬 후에도 보존한다. 직접 시험할 때는 같은 키의 항목에 입력 순서를 나타내는 꼬리표를 붙인다. 예를 들어 `[(2,A), (1,X), (2,B)]`를 첫 값 기준으로 정렬했을 때 `A`가 `B`보다 앞에 있으면 안정이다. 이번 테스트 인터페이스는 정수만 받으므로 이 검사를 직접 실행하지 않고 구현의 동률 처리 방식으로 판별했다.\n\n"
            "| 알고리즘 | 동률 처리 방식 | 판별 |\n"
            "| --- | --- | --- |\n"
            "| Insertion sort | 앞의 값이 현재 값과 같으면 이동을 멈춰 입력 순서를 유지한다. | 안정 |\n"
            "| Quick sort | 분할 중 원소를 교환하므로 같은 키 항목의 상대 순서가 바뀔 수 있다. | 불안정 |\n"
            "| Tree sort | 같은 값을 `occurrences`로 합쳐 항목별 입력 순서를 저장하지 않는다. | 불안정 |\n\n"
            "```mermaid\n"
            "flowchart TD\n"
            "    A[\"고정 시드 20260928\"] --> B[\"기본 입력 10,000개 생성\"]\n"
            "    B --> C[\"동일 입력을 각 정렬에 복사\"]\n"
            "    C --> D[\"qsort 기준으로 정확도 확인\"]\n"
            "    C --> E[\"준비 1회 + clock 7회\"]\n"
            "    E --> F[\"중앙 실행 시간\"]\n"
            "    C --> G[\"별도 SortStats 계측\"]\n"
            "    H[\"정렬된 입력 앞부분 섞기\"] --> I[\"정렬 정도별 시간 측정\"]\n"
            "    J[\"배열 크기 1~128 생성\"] --> K[\"동일 크기 배열 묶음 측정\"]\n"
            "    K --> L[\"크기별 시간과 교차점 분석\"]\n"
            "    D --> M[\"표·그래프·주의사항 기록\"]\n"
            "    F --> M\n"
            "    G --> M\n"
            "    I --> M\n"
            "    L --> M\n"
            "```\n\n"
            "### 측정 결과\n\n"
            "| 알고리즘 | 시간복잡도 (최선 / 평균 / 최악) | 중앙 시간 (ms) | 비교 횟수 | 이동 횟수 | 보조 공간 (bytes) | 최대 재귀 깊이 | 안정성 |\n"
            "| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |\n",
            report);

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
            "    x-axis [\"Insertion\", \"Quick\", \"Tree\"]\n",
            report);
        fprintf(report,
            "    y-axis \"ms\" 0 --> %zu\n    bar [%.3f, %.3f, %.3f]\n```\n\n",
            time_axis_max, results[0].median_ms, results[1].median_ms,
            results[2].median_ms);

        fputs("### 보조 공간 그래프\n\n"
            "```mermaid\n"
            "xychart-beta\n"
            "    title \"알고리즘 보조 공간 (bytes)\"\n"
            "    x-axis [\"Insertion\", \"Quick\", \"Tree\"]\n",
            report);
        fprintf(report,
            "    y-axis \"bytes\" 0 --> %zu\n    bar [%zu, %zu, %zu]\n```\n\n",
            memory_axis_max, results[0].stats.auxiliary_bytes,
            results[1].stats.auxiliary_bytes,
            results[2].stats.auxiliary_bytes);

        writeSortednessResults(report);
        writeSmallArrayResults(report);

    fprintf(report,
            "기본 무작위 10,000개 입력에서 가장 짧은 중앙값은 **%s sort**였고, "
            "기록된 보조 공간이 가장 적은 알고리즘은 **%s sort**였다. "
            "입력 정렬 정도와 크기에 따른 추가 결과는 각각의 표와 교차점에 "
            "정리했다. 측정값은 이 환경과 입력 생성 방식에 따른 관측값이므로 "
            "다른 환경이나 입력 분포에서는 달라질 수 있다.\n\n"
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
        if (!matchesExpected(values, expected, ARRAY_SIZE) ||
            !measureTime(&results[index], source, expected)) {
            fprintf(stderr, "%s sort failed benchmark validation\n",
                    results[index].name);
            return 1;
        }
    }
    if (!runSortednessMeasurements() || !runSmallArrayMeasurements()) {
        fprintf(stderr, "additional benchmark validation failed\n");
        return 1;
    }

    FILE *report = fopen("report/report.md", "w");
    if (report == NULL) {
        perror("report/report.md");
        return 1;
    }
    writeReport(report);
    if (fclose(report) != 0) {
        perror("report/report.md");
        return 1;
    }

    printf("Wrote report/report.md with C-only measurements (n=%d, median of %d runs)\n",
           ARRAY_SIZE, REPEATS);
    return 0;
}
