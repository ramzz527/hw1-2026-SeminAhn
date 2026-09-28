/* 정수 배열 정렬 함수의 공통 인터페이스. */
#ifndef SORT_H
#define SORT_H

typedef void (*SortFunction)(int a[], int n);

/* a[0..n-1]을 제자리에서 오름차순으로 정렬한다. */
void bubbleSort(int a[], int n);
void insertionSort(int a[], int n);
void quickSort(int a[], int n);
void mergeSort(int a[], int n);

#endif /* SORT_H */
