"""유닛 테스트 — 표준 라이브러리의 unittest만 쓴다.

실행: make test-py
"""

import sys
import unittest
from pathlib import Path

# src/를 import 경로에 넣는다. 패키지로 만들지 않아도 되도록.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from sort import SORTS  # noqa: E402

ALL_SORTS = list(SORTS.values())


class TestSorts(unittest.TestCase):
    def test_shuffled(self):
        a = [6, 8, 5, 9, 10, 1, 7, 2, 4, 3]
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                self.assertEqual(sort(a.copy()), [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    def test_already_sorted(self):
        a = [1, 2, 3, 4, 5]
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                self.assertEqual(sort(a.copy()), [1, 2, 3, 4, 5])

    def test_reversed(self):
        a = [5, 4, 3, 2, 1]
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                self.assertEqual(sort(a.copy()), [1, 2, 3, 4, 5])

    def test_duplicates(self):
        a = [3, 1, 3, 1, 2]
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                self.assertEqual(sort(a.copy()), [1, 1, 2, 3, 3])

    def test_single(self):
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                self.assertEqual(sort([42]), [42])

    def test_empty(self):
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                self.assertEqual(sort([]), [])

    def test_sorts_in_place(self):
        a = [3, 1, 2]
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                values = a.copy()
                result = sort(values)
                self.assertIs(result, values)
                self.assertEqual(values, [1, 2, 3])

    def test_metrics(self):
        for sort in ALL_SORTS:
            with self.subTest(sort=sort.__name__):
                metrics = {"comparisons": 0, "moves": 0}
                values = [3, 1, 2]
                sort(values, metrics=metrics)
                self.assertEqual(values, [1, 2, 3])
                self.assertGreater(metrics["comparisons"], 0)
                self.assertGreater(metrics["moves"], 0)

    def test_stability(self):
        records = [(2, "first"), (3, "middle"), (2, "second"), (1, "small")]
        expected = [(1, "small"), (2, "first"), (2, "second"), (3, "middle")]
        for name, sort in SORTS.items():
            with self.subTest(sort=name):
                result = sort(records.copy(), key=lambda record: record[0])
                self.assertEqual([record[0] for record in result], [1, 2, 2, 3])
                if name == "quick":
                    self.assertNotEqual(result, expected)
                else:
                    self.assertEqual(result, expected)


if __name__ == "__main__":
    unittest.main()
