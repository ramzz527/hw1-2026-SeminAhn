#include "sort.h"
#include <stdlib.h>

static void mergeSortRange(int a[], int buffer[], int low, int high,
                           unsigned int depth, SortStats *stats) {
    if (stats != NULL && depth > stats->max_recursion_depth) {
        stats->max_recursion_depth = depth;
    }
    if (high - low < 2) {
        return;
    }

    int middle = low + (high - low) / 2;
    mergeSortRange(a, buffer, low, middle, depth + 1, stats);
    mergeSortRange(a, buffer, middle, high, depth + 1, stats);

    int left = low;
    int right = middle;
    for (int output = low; output < high; output++) {
        if (left < middle && right < high) {
            if (stats != NULL) {
                stats->comparisons++;
            }
        }
        if (left < middle && (right >= high || a[left] <= a[right])) {
            buffer[output] = a[left++];
        } else {
            buffer[output] = a[right++];
        }
        if (stats != NULL) {
            stats->moves++;
        }
    }
    for (int i = low; i < high; i++) {
        a[i] = buffer[i];
        if (stats != NULL) {
            stats->moves++;
        }
    }
}

void mergeSort(int a[], int n, SortStats *stats) {
    if (stats != NULL) {
        *stats = (SortStats){0};
    }
    if (n < 2) {
        return;
    }

    if (stats != NULL) {
        stats->auxiliary_bytes = (size_t)n * sizeof(int);
    }
    int *buffer = malloc((size_t)n * sizeof(*buffer));
    if (buffer == NULL) {
        return;
    }
    mergeSortRange(a, buffer, 0, n, 1, stats);
    free(buffer);
}
