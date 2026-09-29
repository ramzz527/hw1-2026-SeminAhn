"""정렬 구현과 공통 Python 인터페이스."""


def insertion_sort(a, key=None, metrics=None):
    """a를 제자리에서 안정적으로 오름차순 정렬한다."""
    if key is None:
        key = lambda value: value

    for index in range(1, len(a)):
        value = a[index]
        if metrics is not None:
            metrics["moves"] += 1
        value_key = key(value)
        position = index - 1

        while position >= 0:
            if metrics is not None:
                metrics["comparisons"] += 1
            if key(a[position]) <= value_key:
                break
            a[position + 1] = a[position]
            if metrics is not None:
                metrics["moves"] += 1
            position -= 1
        a[position + 1] = value
        if metrics is not None:
            metrics["moves"] += 1
    return a


def quick_sort(a, key=None, metrics=None):
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
            if metrics is not None:
                metrics["moves"] += 1
            less = low
            scan = low
            greater = high

            while scan <= greater:
                scan_key = key(a[scan])
                if metrics is not None:
                    metrics["comparisons"] += 1
                if scan_key < pivot_key:
                    a[less], a[scan] = a[scan], a[less]
                    if metrics is not None:
                        metrics["moves"] += 3
                    less += 1
                    scan += 1
                else:
                    if metrics is not None:
                        metrics["comparisons"] += 1
                    if scan_key > pivot_key:
                        a[scan], a[greater] = a[greater], a[scan]
                        if metrics is not None:
                            metrics["moves"] += 3
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


def merge_sort(a, key=None, metrics=None):
    """a를 안정 병합 정렬로 오름차순 정렬한다."""
    if key is None:
        key = lambda value: value

    length = len(a)
    buffer = [None] * length
    width = 1
    while width < length:
        for start in range(0, length, 2 * width):
            middle = min(start + width, length)
            end = min(start + 2 * width, length)
            left = start
            right = middle
            output = start

            while left < middle and right < end:
                if metrics is not None:
                    metrics["comparisons"] += 1
                if key(a[left]) <= key(a[right]):
                    buffer[output] = a[left]
                    if metrics is not None:
                        metrics["moves"] += 1
                    left += 1
                else:
                    buffer[output] = a[right]
                    if metrics is not None:
                        metrics["moves"] += 1
                    right += 1
                output += 1

            while left < middle:
                buffer[output] = a[left]
                if metrics is not None:
                    metrics["moves"] += 1
                left += 1
                output += 1
            while right < end:
                buffer[output] = a[right]
                if metrics is not None:
                    metrics["moves"] += 1
                right += 1
                output += 1

        a[:] = buffer
        if metrics is not None:
            metrics["moves"] += length
        width *= 2
    return a


SORTS = {
    "insertion": insertion_sort,
    "quick": quick_sort,
    "merge": merge_sort,
}
