/* 실행: make run-c */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sort.h"

enum { BASE_ARRAY_SIZE = 2000, MAX_ARRAY_SIZE = 8000, REPEATS = 3 };

typedef struct {
    const char *name;
    SortFunction sort;
    int stable;
} SortEntry;

typedef void (*InputGenerator)(int values[], int count);

typedef struct {
    const char *group;
    const char *shape;
    const char *group_label;
    const char *shape_label;
    int count;
    InputGenerator generate;
} Experiment;

static const SortEntry sorts[] = {
    {"insertion", insertionSort, 1},
    {"quick", quickSort, 0},
    {"tree", treeSort, 0},
};

static void makeRandomInput(int values[], int count) {
    uint32_t state = 20260928u;
    for (int index = 0; index < count; index++) {
        state = state * 1664525u + 1013904223u;
        values[index] = (int)(state % 1000000u);
    }
}

static void makeSortedInput(int values[], int count) {
    for (int index = 0; index < count; index++) {
        values[index] = index;
    }
}

static void makeReverseInput(int values[], int count) {
    for (int index = 0; index < count; index++) {
        values[index] = count - index - 1;
    }
}

static void makeDuplicateHeavyInput(int values[], int count) {
    uint32_t state = 20260929u;
    for (int index = 0; index < count; index++) {
        state = state * 1664525u + 1013904223u;
        values[index] = (int)(state % 8u);
    }
}

static int compareInts(const void *left, const void *right) {
    int left_value = *(const int *)left;
    int right_value = *(const int *)right;
    return (left_value > right_value) - (left_value < right_value);
}

static int isSorted(const int values[], int count) {
    for (int index = 1; index < count; index++) {
        if (values[index - 1] > values[index]) {
            return 0;
        }
    }
    return 1;
}

static double median(double samples[REPEATS]) {
    for (int index = 0; index < REPEATS; index++) {
        for (int next = index + 1; next < REPEATS; next++) {
            if (samples[next] < samples[index]) {
                double temporary = samples[index];
                samples[index] = samples[next];
                samples[next] = temporary;
            }
        }
    }
    return samples[REPEATS / 2];
}

static int measureTime(const SortEntry *entry, const int source[],
                       const int expected[], int count, double *elapsed_ms) {
    double samples[REPEATS];
    int values[MAX_ARRAY_SIZE];

    for (int repeat = 0; repeat < REPEATS; repeat++) {
        memcpy(values, source, sizeof(int) * (size_t)count);
        clock_t start = clock();
        entry->sort(values, count, NULL);
        clock_t end = clock();
        if (start == (clock_t)-1 || end == (clock_t)-1 || end < start ||
            memcmp(values, expected, sizeof(int) * (size_t)count) != 0) {
            return 0;
        }
        samples[repeat] = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
    }
    *elapsed_ms = median(samples);
    return 1;
}

static int runExperiment(FILE *csv, const Experiment *experiment) {
    int source[MAX_ARRAY_SIZE];
    int expected[MAX_ARRAY_SIZE];
    experiment->generate(source, experiment->count);
    memcpy(expected, source, sizeof(int) * (size_t)experiment->count);
    qsort(expected, (size_t)experiment->count, sizeof(expected[0]), compareInts);

    printf("\n%s: %s (n=%d, 중앙값 %d회)\n", experiment->group_label,
           experiment->shape_label, experiment->count, REPEATS);
    printf("알고리즘   시간(ms)   비교   이동   메모리 재귀깊이 정렬 안정성\n");

    for (size_t index = 0; index < sizeof(sorts) / sizeof(sorts[0]); index++) {
        const SortEntry *entry = &sorts[index];
        int values[MAX_ARRAY_SIZE];
        memcpy(values, source, sizeof(int) * (size_t)experiment->count);

        SortStats stats = {0};
        entry->sort(values, experiment->count, &stats);
        int sorted = memcmp(values, expected,
                            sizeof(int) * (size_t)experiment->count) == 0;
        double elapsed_ms;
        if (!sorted ||
            !measureTime(entry, source, expected, experiment->count,
                         &elapsed_ms)) {
            fprintf(stderr, "%s sort failed %s/%s validation\n",
                    entry->name, experiment->group, experiment->shape);
            return 0;
        }

        printf("%-9s %7.3f %10zu %10zu %8zu %5u %6s %6s\n",
               entry->name, elapsed_ms, stats.comparisons, stats.moves,
               stats.auxiliary_bytes, stats.max_recursion_depth,
               isSorted(values, experiment->count) ? "yes" : "no",
               entry->stable ? "yes" : "no");
        if (fprintf(csv, "%s,%s,%d,%d,%s,%.3f,%zu,%zu,%zu,%u,%s,%s\n",
                    experiment->group, experiment->shape, experiment->count,
                    REPEATS, entry->name, elapsed_ms, stats.comparisons,
                    stats.moves, stats.auxiliary_bytes,
                    stats.max_recursion_depth, sorted ? "yes" : "no",
                    entry->stable ? "yes" : "no") < 0) {
            perror("report/results.csv");
            return 0;
        }
    }
    return 1;
}

int main(void) {
    static const Experiment experiments[] = {
        {"input_shape", "random", "입력 모양별 비교", "무작위", BASE_ARRAY_SIZE,
         makeRandomInput},
        {"input_shape", "sorted", "입력 모양별 비교", "정렬됨", BASE_ARRAY_SIZE,
         makeSortedInput},
        {"input_shape", "reverse", "입력 모양별 비교", "역순", BASE_ARRAY_SIZE,
         makeReverseInput},
        {"input_shape", "duplicate_heavy", "입력 모양별 비교", "중복 다수",
         BASE_ARRAY_SIZE, makeDuplicateHeavyInput},
        {"random_size", "random", "무작위 입력 크기별 비교", "무작위",
         1000, makeRandomInput},
        {"random_size", "random", "무작위 입력 크기별 비교", "무작위",
         2000, makeRandomInput},
        {"random_size", "random", "무작위 입력 크기별 비교", "무작위",
         4000, makeRandomInput},
        {"random_size", "random", "무작위 입력 크기별 비교", "무작위",
         8000, makeRandomInput},
    };

    FILE *csv = fopen("report/results.csv", "w");
    if (csv == NULL) {
        perror("report/results.csv");
        return 1;
    }
    fputs("experiment,input_shape,array_size,repeats,algorithm,time_ms,"
          "comparisons,moves,auxiliary_bytes,max_recursion_depth,is_sorted,stable\n",
          csv);

    for (size_t index = 0;
         index < sizeof(experiments) / sizeof(experiments[0]); index++) {
        if (!runExperiment(csv, &experiments[index])) {
            fclose(csv);
            return 1;
        }
    }
    if (fclose(csv) != 0) {
        perror("report/results.csv");
        return 1;
    }
    printf("\n비교: 비교 횟수, 이동 횟수, 보조 공간(B), 최대 재귀 깊이, 정렬 여부, 안정성\n");
    printf("보조 공간은 입력과 호출 스택을 제외하며, report/results.csv는 매 실행 시 덮어씁니다.\n");
    return 0;
}
