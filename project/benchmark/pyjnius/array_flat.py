"""Flat (1D) array conversion, pyjnius side, at increasing sizes. Swept
across four primitive element types (int32, int64, float32, float64), same
sweep as jpype/array_flat.py, jpy/array_flat.py, so a per-type gap (not
just a per-size one) shows up if there is one. Companion: jpype/array_flat.py,
jpy/array_flat.py, jep/array_flat.py -- same sizes, using the shared
jpype.benchmark.DeepBench test class. See ../array_multidim.py and
../README.md.

pyjnius has no buffer->array push at all, at any size -- confirmed
empirically, not assumed: passing a numpy array as an argument where a
Java array is expected raises `JavaException('Expecting a python
list/tuple, got array(...)')` unconditionally. This is stricter than
jep (which does have a real numpy fast path for a flat 1D target) and
stricter than jpy/jpype (which both accept a buffer-protocol object).
So only three rows exist here, not four, for every type in the sweep:
  - "push, list->array": DeepBench.void{Type}Array(list) -- the only push
    path pyjnius has for arrays, period.
  - "pull, array->list": DeepBench.make{Type}Array(n) -- unlike the other
    three libraries, pyjnius doesn't return a wrapper array object here
    at all. The Java {type}[] return value comes back already converted to
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

Writes project/benchmark/pyjnius/array_flat_results.csv alongside the
printed output.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/array_flat.py \
        [classes_dir] [harness_dir]
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import jnius_config

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'
jnius_config.set_classpath(classes_dir, harness_dir)

import numpy as np
from jnius import autoclass

DeepBench = autoclass('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]

# (label, sum{Type}Array, make{Type}Array)
TYPES = [
    ('int', DeepBench.voidIntArray, DeepBench.makeIntArray),
    ('long', DeepBench.voidLongArray, DeepBench.makeLongArray),
    ('float', DeepBench.voidFloatArray, DeepBench.makeFloatArray),
    ('double', DeepBench.voidDoubleArray, DeepBench.makeDoubleArray),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_flat_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, direction, source, dtype):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_flat', direction=direction, source=source,
                   dtype=dtype, size=total_elements, n=n, best_ns=best, median_ns=median)


for label, sumfn, makefn in TYPES:
    print(f"=== pyjnius: list->array, flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        # Python-level element type matches the target array's own kind --
        # exercises each type's own homogeneous push, not a widening
        # conversion from a different Python type.
        lst = [float(i) for i in range(size)] if label in ('float', 'double') else list(range(size))
        run(f"list->array {label}[{size}], fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label)

    if label in ('float', 'double'):
        print(f"=== pyjnius: list->array, flat, push (Python -> Java), {label}, widening from int ===")
        for size in SIZES:
            # A plain Python int list pushed into a float[]/double[]
            # target -- idiomatic, and not the same benchmark as the
            # homogeneous-type row above.
            lst = list(range(size))
            run(f"list->array {label}[{size}], widening from int",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
                'push', 'list_widen', label)

    print(f"=== pyjnius: array->list, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->list {label}[{size}]",
            lambda size=size, makefn=makefn: makefn(size), size,
            'pull', 'list', label)

    print(f"=== pyjnius: array->buffer, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->buffer {label}[{size}]",
            lambda size=size, makefn=makefn: np.asarray(makefn(size)), size,
            'pull', 'buffer', label)

csv_log.close()
