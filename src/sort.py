"""정렬 구현과 공통 Python 인터페이스."""


def bubble_sort(a):
    """a를 제자리에서 오름차순으로 정렬한다."""
    n = len(a)
    for i in range(n - 1):
        swapped = False
        # 한 번 훑을 때마다 가장 큰 값이 뒤로 밀려 자리를 잡는다.
        for j in range(n - 1 - i):
            if a[j] > a[j + 1]:
                a[j], a[j + 1] = a[j + 1], a[j]
                swapped = True
        # 한 바퀴 동안 교환이 없었다면 이미 정렬된 것이다.
        if not swapped:
            return a
    return a


def insertion_sort(a, key=None):
    """a를 제자리에서 안정적으로 오름차순 정렬한다."""
    if key is None:
        key = lambda value: value

    for index in range(1, len(a)):
        value = a[index]
        value_key = key(value)
        position = index - 1
        while position >= 0 and key(a[position]) > value_key:
            a[position + 1] = a[position]
            position -= 1
        a[position + 1] = value
    return a


def quick_sort(a, key=None):
    """a를 제자리에서 불안정 퀵 정렬로 오름차순 정렬한다."""
    if key is None:
        key = lambda value: value

    if len(a) < 2:
        return a

    pending = [(0, len(a) - 1)]
    while pending:
        low, high = pending.pop()
        while low < high:
            pivot_key = key(a[(low + high) // 2])
            less = low
            scan = low
            greater = high

            while scan <= greater:
                scan_key = key(a[scan])
                if scan_key < pivot_key:
                    a[less], a[scan] = a[scan], a[less]
                    less += 1
                    scan += 1
                elif scan_key > pivot_key:
                    a[scan], a[greater] = a[greater], a[scan]
                    greater -= 1
                else:
                    scan += 1

            left_size = less - low
            right_size = high - greater
            if left_size < right_size:
                if greater + 1 < high:
                    pending.append((greater + 1, high))
                high = less - 1
            else:
                if low < less - 1:
                    pending.append((low, less - 1))
                low = greater + 1
    return a


def merge_sort(a, key=None):
    """a를 안정 병합 정렬로 오름차순 정렬한다."""
    if key is None:
        key = lambda value: value

    length = len(a)
    buffer = list(a)
    width = 1
    while width < length:
        for start in range(0, length, 2 * width):
            middle = min(start + width, length)
            end = min(start + 2 * width, length)
            left = start
            right = middle
            output = start

            while left < middle and right < end:
                if key(a[left]) <= key(a[right]):
                    buffer[output] = a[left]
                    left += 1
                else:
                    buffer[output] = a[right]
                    right += 1
                output += 1

            while left < middle:
                buffer[output] = a[left]
                left += 1
                output += 1
            while right < end:
                buffer[output] = a[right]
                right += 1
                output += 1

        a[:] = buffer
        width *= 2
    return a


SORTS = {
    "insertion": insertion_sort,
    "quick": quick_sort,
    "merge": merge_sort,
}
