/* 실행: make run-c */
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "sort.h"

enum { ARRAY_SIZE = 2000, REPEATS = 3 };

typedef struct {
    const char *name;
    SortFunction sort;
    int stable;
} SortEntry;

static const SortEntry sorts[] = {
    {"insertion", insertionSort, 1},
    {"quick", quickSort, 0},
    {"merge", mergeSort, 1},
};

static void makeInput(int values[], int count) {
    uint32_t state = 20260928u;
    for (int index = 0; index < count; index++) {
        state = state * 1664525u + 1013904223u;
        values[index] = (int)(state % 1000000u);
    }
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

static double measureTime(const SortEntry *entry, const int source[]) {
    double samples[REPEATS];
    int values[ARRAY_SIZE];

    for (int repeat = 0; repeat < REPEATS; repeat++) {
        for (int index = 0; index < ARRAY_SIZE; index++) {
            values[index] = source[index];
        }
        clock_t start = clock();
        entry->sort(values, ARRAY_SIZE, NULL);
        clock_t end = clock();
        samples[repeat] = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;
    }
    return median(samples);
}

int main(void) {
    int source[ARRAY_SIZE];
    makeInput(source, ARRAY_SIZE);

        printf("C (n=%d, median of %d runs)\n", ARRAY_SIZE, REPEATS);
        printf("알고리즘   시간(ms)   비교   이동   메모리 재귀깊이 정렬 안정성\n");

    for (size_t index = 0; index < sizeof(sorts) / sizeof(sorts[0]); index++) {
        const SortEntry *entry = &sorts[index];
        int values[ARRAY_SIZE];
        for (int item = 0; item < ARRAY_SIZE; item++) {
            values[item] = source[item];
        }

        SortStats stats = {0};
        entry->sort(values, ARRAY_SIZE, &stats);
         printf("%-9s %7.3f %10zu %10zu %8zu %5u %6s %6s\n",
               entry->name, measureTime(entry, source), stats.comparisons,
               stats.moves, stats.auxiliary_bytes, stats.max_recursion_depth,
             isSorted(values, ARRAY_SIZE) ? "yes" : "no",
             entry->stable ? "yes" : "no");
    }
        printf("cmp=element comparisons; moves=element assignments including scratch copies.\n");
        printf("aux(B) excludes input and call-stack storage.\n");
    return 0;
}
