"""Flat (1D) array conversion, pyjnius side, at increasing sizes.
Companion: jpype/array_flat.py, jpy/array_flat.py, jep/array_flat.py --
same sizes, using the shared jpype.benchmark.DeepBench test class. See
../array_multidim.py and ../README.md.

pyjnius has no buffer->array push at all, at any size -- confirmed
empirically, not assumed: passing a numpy array as an argument where a
Java array is expected raises `JavaException('Expecting a python
list/tuple, got array(...)')` unconditionally. This is stricter than
jep (which does have a real numpy fast path for a flat 1D target) and
stricter than jpy/jpype (which both accept a buffer-protocol object).
So only two rows exist here, not four:
  - "push, list->array": DeepBench.sumIntArray(list) -- the only push
    path pyjnius has for arrays, period.
  - "pull, array->list": DeepBench.makeIntArray(n) -- unlike the other
    three libraries, pyjnius doesn't return a wrapper array object here
    at all. The Java int[] return value comes back already converted to
    a genuine, fully-materialized Python list (confirmed:
    `type(DeepBench.makeIntArray(10))` is `list`) -- so this row *is*
    the raw return value, with no separate list()/wrapper-unwrap step to
    benchmark.
  - "pull, array->buffer": np.asarray() applied to that same already-a-
    list return value. Since pyjnius never hands back anything but a
    plain list, this can only ever be array->list's cost plus an extra
    numpy conversion step on top -- there is no bulk Java-array-to-numpy
    path in pyjnius to measure, so expect this row to be strictly slower
    than array->list at every size, not faster (the opposite of jpype's/
    jpy's array->buffer, which have a real buffer read to win with).

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/array_flat.py \
        [classes_dir] [harness_dir]
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jnius_config

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'
jnius_config.set_classpath(classes_dir, harness_dir)

import numpy as np
from jnius import autoclass

DeepBench = autoclass('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


print("=== pyjnius: list->array, flat, push (Python -> Java) ===")
for size in SIZES:
    lst = list(range(size))
    run(f"list->array int[{size}], fresh",
        lambda lst=lst: DeepBench.sumIntArray(lst), size)

print("=== pyjnius: array->list, flat, pull (Java -> Python) ===")
for size in SIZES:
    run(f"array->list int[{size}]",
        lambda size=size: DeepBench.makeIntArray(size), size)

print("=== pyjnius: array->buffer, flat, pull (Java -> Python) ===")
for size in SIZES:
    run(f"array->buffer int[{size}]",
        lambda size=size: np.asarray(DeepBench.makeIntArray(size)), size)
