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
    {"bubble", bubbleSort},
    {"insertion", insertionSort},
    {"quick", quickSort},
    {"merge", mergeSort},
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
    entry->sort(actual, n);
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

int main(void) {
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

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
