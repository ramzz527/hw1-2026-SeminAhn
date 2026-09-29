/* 정수 배열 정렬 함수의 공통 인터페이스. */
#ifndef SORT_H
#define SORT_H

#include <stddef.h>

typedef struct {
	size_t comparisons;
	size_t moves;
	size_t auxiliary_bytes;
	unsigned int max_recursion_depth;
} SortStats;

typedef void (*SortFunction)(int a[], int n, SortStats *stats);

/* a[0..n-1]을 제자리에서 오름차순으로 정렬한다. stats는 NULL일 수 있다. */
void insertionSort(int a[], int n, SortStats *stats);
void quickSort(int a[], int n, SortStats *stats);
void mergeSort(int a[], int n, SortStats *stats);

#endif /* SORT_H */
