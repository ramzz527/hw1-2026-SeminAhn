#include "sort.h"

static void quickSortRange(int a[], int low, int high, unsigned int depth,
                           SortStats *stats) {
    if (low < high && stats != NULL && depth > stats->max_recursion_depth) {
        stats->max_recursion_depth = depth;
    }

    while (low < high) {
        int pivot = a[low + (high - low) / 2];
        if (stats != NULL) {
            stats->moves++;
        }
        int less = low;
        int scan = low;
        int greater = high;

        while (scan <= greater) {
            if (stats != NULL) {
                stats->comparisons++;
            }
            if (a[scan] < pivot) {
                int tmp = a[less];
                if (stats != NULL) {
                    stats->moves++;
                }
                a[less++] = a[scan];
                if (stats != NULL) {
                    stats->moves++;
                }
                a[scan++] = tmp;
                if (stats != NULL) {
                    stats->moves++;
                }
            } else if (a[scan] > pivot) {
                if (stats != NULL) {
                    stats->comparisons++;
                }
                int tmp = a[greater];
                if (stats != NULL) {
                    stats->moves++;
                }
                a[greater--] = a[scan];
                if (stats != NULL) {
                    stats->moves++;
                }
                a[scan] = tmp;
                if (stats != NULL) {
                    stats->moves++;
                }
            } else {
                if (stats != NULL) {
                    stats->comparisons++;
                }
                scan++;
            }
        }

        if (less - low < high - greater) {
            quickSortRange(a, low, less - 1, depth + 1, stats);
            low = greater + 1;
        } else {
            quickSortRange(a, greater + 1, high, depth + 1, stats);
            high = less - 1;
        }
    }
}

void quickSort(int a[], int n, SortStats *stats) {
    if (stats != NULL) {
        *stats = (SortStats){0};
    }
    if (n > 1) {
        if (stats != NULL) {
            stats->auxiliary_bytes = 2 * sizeof(int);
        }
        quickSortRange(a, 0, n - 1, 1, stats);
    }
}
