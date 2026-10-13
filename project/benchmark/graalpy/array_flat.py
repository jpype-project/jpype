"""Flat (1D) array conversion, GraalPy side, at increasing sizes. Swept
across four primitive element types (int32, int64, float32, float64), same
sweep as jpype/array_flat.py, jpy/array_flat.py, jep/array_flat.py,
pyjnius/array_flat.py. See ../array_multidim.py and ../README.md.

GraalPy has no buffer->array push at all, at any size -- confirmed
empirically, not assumed: passing a numpy array where a Java array
argument is expected raises `TypeError('invalid instantiation of foreign
object')` unconditionally. Unlike pyjnius's identical gap (see
../pyjnius/array_flat.py), this isn't treated as a "no path" stub here:
converting a numpy array to a Java array is basic orchestration, so
../_arrayutil.py's build_manual() emulates the missing push by hand
(genuine Java array, filled element by element from the numpy source) --
a real, measured "buffer->array (manual)" row, not a documented gap. It's
a pure per-element Python-level loop, no bulk copy, which is exactly the
kind of hot uniform loop a real JIT is supposed to be good at optimizing.

Four rows per type, matching jpype's/jpy's array_flat.py exactly:
  - "push, list->array": DeepBench.sum{Type}Array(list) -- GraalPy's one
    automatic push path.
  - "push, buffer->array (manual)": build_manual() assembles a genuine
    Java array from the numpy source first, then the same sum{Type}Array
    call as above -- the emulated push path (see ../_arrayutil.py).
  - "pull, array->list": DeepBench.make{Type}Array(n) comes back as a
    `polyglot.ForeignList`; list(...) walks it via the sequence protocol,
    one polyglot call per element. GraalPy has no toList()-equivalent
    bulk pull method, so unlike jpype's array_flat.py there is no fifth
    "array->list via toList()" row.
  - "pull, array->buffer": np.asarray() on that same ForeignList.

Writes project/benchmark/graalpy/array_flat_results.csv alongside the
printed output.

Usage: run via the Bench launcher with DeepBench on the classpath (see
../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog
from _arrayutil import build_manual, calls_for_manual

import numpy as np
import java

DeepBench = java.type('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]

# (label, numpy dtype, sum{Type}Array, make{Type}Array)
TYPES = [
    ('int', np.dtype('int32'), DeepBench.sumIntArray, DeepBench.makeIntArray),
    ('long', np.dtype('int64'), DeepBench.sumLongArray, DeepBench.makeLongArray),
    ('float', np.dtype('float32'), DeepBench.sumFloatArray, DeepBench.makeFloatArray),
    ('double', np.dtype('float64'), DeepBench.sumDoubleArray, DeepBench.makeDoubleArray),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_flat_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements, direction, source, dtype, manual=False):
    n, warmup = calls_for_manual(total_elements) if manual else calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_flat', direction=direction, source=source,
                   dtype=dtype, size=total_elements, n=n, best_ns=best, median_ns=median)


for label, dtype, sumfn, makefn in TYPES:
    print(f"=== GraalPy: list->array, flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        # Python-level element type matches the target array's own kind --
        # exercises each type's own homogeneous push, not a widening
        # conversion from a different Python type.
        lst = [float(i) for i in range(size)] if dtype.kind == 'f' else list(range(size))
        run(f"list->array {label}[{size}], fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label)

    print(f"=== GraalPy: buffer->array (manual), flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        arr = np.arange(size, dtype=dtype)
        run(f"buffer->array {label}[{size}], manual, fresh",
            lambda arr=arr, sumfn=sumfn, label=label:
                sumfn(build_manual(arr, 1, label)), size,
            'push', 'buffer_manual', label, manual=True)

    print(f"=== GraalPy: array->list, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->list {label}[{size}]",
            lambda size=size, makefn=makefn: list(makefn(size)), size,
            'pull', 'list', label)

    print(f"=== GraalPy: array->buffer, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->buffer {label}[{size}]",
            lambda size=size, makefn=makefn: np.asarray(makefn(size)), size,
            'pull', 'buffer', label)

csv_log.close()
