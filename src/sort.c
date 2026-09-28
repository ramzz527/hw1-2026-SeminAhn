#include "sort.h"
#include <stdlib.h>

void bubbleSort(int a[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        /* 한 번 훑을 때마다 가장 큰 값이 뒤로 밀려 자리를 잡는다. */
        for (int j = 0; j < n - 1 - i; j++) {
            if (a[j] > a[j + 1]) {
                int tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
                swapped = 1;
            }
        }
        /* 한 바퀴 동안 교환이 없었다면 이미 정렬된 것이다. */
        if (!swapped) {
            return;
        }
    }
}

void insertionSort(int a[], int n) {
    for (int i = 1; i < n; i++) {
        int value = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > value) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = value;
    }
}

static void quickSortRange(int a[], int low, int high) {
    while (low < high) {
        int pivot = a[low + (high - low) / 2];
        int less = low;
        int scan = low;
        int greater = high;

        while (scan <= greater) {
            if (a[scan] < pivot) {
                int tmp = a[less];
                a[less++] = a[scan];
                a[scan++] = tmp;
            } else if (a[scan] > pivot) {
                int tmp = a[greater];
                a[greater--] = a[scan];
                a[scan] = tmp;
            } else {
                scan++;
            }
        }

        if (less - low < high - greater) {
            quickSortRange(a, low, less - 1);
            low = greater + 1;
        } else {
            quickSortRange(a, greater + 1, high);
            high = less - 1;
        }
    }
}

void quickSort(int a[], int n) {
    if (n > 1) {
        quickSortRange(a, 0, n - 1);
    }
}

static void mergeSortRange(int a[], int buffer[], int low, int high) {
    if (high - low < 2) {
        return;
    }

    int middle = low + (high - low) / 2;
    mergeSortRange(a, buffer, low, middle);
    mergeSortRange(a, buffer, middle, high);

    int left = low;
    int right = middle;
    for (int output = low; output < high; output++) {
        if (left < middle && (right >= high || a[left] <= a[right])) {
            buffer[output] = a[left++];
        } else {
            buffer[output] = a[right++];
        }
    }
    for (int i = low; i < high; i++) {
        a[i] = buffer[i];
    }
}

void mergeSort(int a[], int n) {
    if (n < 2) {
        return;
    }

    int *buffer = malloc((size_t)n * sizeof(*buffer));
    if (buffer == NULL) {
        return;
    }
    mergeSortRange(a, buffer, 0, n);
    free(buffer);
}
