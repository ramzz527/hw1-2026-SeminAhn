#include "sort.h"

void insertionSort(int a[], int n, SortStats *stats) {
    if (stats != NULL) {
        *stats = (SortStats){0};
        if (n > 1) {
            stats->auxiliary_bytes = sizeof(int);
        }
    }

    for (int i = 1; i < n; i++) {
        int value = a[i];
        if (stats != NULL) {
            stats->moves++;
        }
        int j = i - 1;

        while (j >= 0) {
            if (stats != NULL) {
                stats->comparisons++;
            }
            if (a[j] <= value) {
                break;
            }
            a[j + 1] = a[j];
            if (stats != NULL) {
                stats->moves++;
            }
            j--;
        }
        a[j + 1] = value;
        if (stats != NULL) {
            stats->moves++;
        }
    }
}
