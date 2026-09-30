/* 유닛 테스트 — 외부 프레임워크 없이 표준 C만 쓴다.
 * 실행: make test-c
 */
#include <stdio.h>
#include <string.h>
#include "sort.h"

static int checks = 0;
static int failures = 0;

typedef struct {
    const char *name;
    SortFunction sort;
} SortEntry;

static const SortEntry sorts[] = {
    {"insertion", insertionSort},
    {"quick", quickSort},
    {"tree", treeSort},
};

static void printArray(const char *label, const int a[], int n) {
    printf("      %s:", label);
    for (int i = 0; i < n; i++) {
        printf(" %d", a[i]);
    }
    printf("\n");
}

/* input을 정렬한 결과가 want와 같은지 본다. */
static void expectSorted(const char *caseName, const SortEntry *entry,
                         const int input[], const int want[], int n) {
    int actual[n > 0 ? n : 1];
    if (n > 0) {
        memcpy(actual, input, (size_t)n * sizeof(int));
    }
    checks++;
    entry->sort(actual, n, NULL);
    if (n > 0 && memcmp(actual, want, (size_t)n * sizeof(int)) != 0) {
        failures++;
        printf("FAIL  %s (%s)\n", caseName, entry->name);
        printArray("got ", actual, n);
        printArray("want", want, n);
        return;
    }
    printf("ok    %s (%s)\n", caseName, entry->name);
}

static void expectAllSorted(const char *caseName, const int input[],
                            const int want[], int n) {
    for (size_t i = 0; i < sizeof(sorts) / sizeof(sorts[0]); i++) {
        expectSorted(caseName, &sorts[i], input, want, n);
    }
}

static void expectMetrics(void) {
    const int want[] = {1, 2, 3};

    for (size_t index = 0; index < sizeof(sorts) / sizeof(sorts[0]); index++) {
        int actual[] = {3, 1, 2};
        SortStats stats = {0};
        sorts[index].sort(actual, 3, &stats);
        checks++;
        if (memcmp(actual, want, sizeof(want)) != 0 ||
            stats.comparisons == 0 || stats.moves == 0 ||
            stats.auxiliary_bytes == 0) {
            failures++;
            printf("FAIL  metrics (%s)\n", sorts[index].name);
        } else {
            printf("ok    metrics (%s)\n", sorts[index].name);
        }
    }
}

int main(void) {
    expectMetrics();
    {
        int a[] = {6, 8, 5, 9, 10, 1, 7, 2, 4, 3};
        const int want[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        expectAllSorted("섞인 배열", a, want, 10);
    }
    {
        int a[] = {1, 2, 3, 4, 5};
        const int want[] = {1, 2, 3, 4, 5};
        expectAllSorted("이미 정렬된 배열", a, want, 5);
    }
    {
        int a[] = {5, 4, 3, 2, 1};
        const int want[] = {1, 2, 3, 4, 5};
        expectAllSorted("역순 배열", a, want, 5);
    }
    {
        int a[] = {3, 1, 3, 1, 2};
        const int want[] = {1, 1, 2, 3, 3};
        expectAllSorted("중복이 있는 배열", a, want, 5);
    }
    {
        int a[] = {42};
        const int want[] = {42};
        expectAllSorted("원소 하나", a, want, 1);
    }
    {
        /* n = 0이면 배열을 건드리지 않는다. 초기화해 두어야 경고가 없다. */
        int a[1] = {0};
        const int want[1] = {0};
        expectAllSorted("빈 배열", a, want, 0);
    }
    {
        int a[] = {2, 1};
        const int want[] = {1, 2};
        expectAllSorted("두 원소 역순", a, want, 2);
    }
    {
        int a[] = {-3, 5, -1, 0, -2};
        const int want[] = {-3, -2, -1, 0, 5};
        expectAllSorted("음수가 있는 배열", a, want, 5);
    }
    {
        int a[] = {7, 7, 7, 7};
        const int want[] = {7, 7, 7, 7};
        expectAllSorted("모든 원소가 같은 배열", a, want, 4);
    }
    {
        int a[] = {4, 1, 4, 2, 4, 3, 4};
        const int want[] = {1, 2, 3, 4, 4, 4, 4};
        expectAllSorted("반복값이 섞인 배열", a, want, 7);
    }
    {
        int a[] = {2147483647, 0, -2147483647, 1};
        const int want[] = {-2147483647, 0, 1, 2147483647};
        expectAllSorted("큰 정수 범위 배열", a, want, 4);
    }
    {
        int a[] = {1, 2, 3, 5, 4, 6, 7};
        const int want[] = {1, 2, 3, 4, 5, 6, 7};
        expectAllSorted("거의 정렬된 배열", a, want, 7);
    }
    {
        int a[] = {9, 1, 8, 2, 7, 3, 6, 4, 5};
        const int want[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        expectAllSorted("양 끝값 교차 배열", a, want, 9);
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
