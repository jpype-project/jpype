"""Multi-dimensional array conversion, pyjnius side, 2D through 5D,
holding total element count fixed at each depth (matching
array_flat.py's sizes). Companion: jpype/array_multidim.py,
jpy/array_multidim.py, jep/array_multidim.py -- same operations and
depths, using the shared jpype.benchmark.DeepBench test class. See
../array_flat.py and ../README.md.

Same story as array_flat.py, confirmed at every depth: pyjnius rejects
numpy input for array arguments unconditionally (`JavaException(
'Expecting a python list/tuple, got array(...)')`), so there's no
buffer->array push row here either -- only "list->array" (nested Python
lists, which pyjnius's general per-element conversion does accept and
recurse through correctly). And every pull direction returns an already
fully-materialized, recursively nested plain Python list -- confirmed
for 2D (`type(DeepBench.make2DIntArray(4))` is `list`, and so is
`type(DeepBench.make2DIntArray(4)[0])`) -- so "array->list" is again
just the raw return value, and "array->buffer" is that same value with
an extra `np.asarray()` step on top, never faster.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/array_multidim.py \
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

DIMS = [2, 3, 4, 5]
SUM_BY_DIMS = {
    2: DeepBench.sum2DIntArray,
    3: DeepBench.sum3DIntArray,
    4: DeepBench.sum4DIntArray,
    5: DeepBench.sum5DIntArray,
}
MAKE_BY_DIMS = {
    2: DeepBench.make2DIntArray,
    3: DeepBench.make3DIntArray,
    4: DeepBench.make4DIntArray,
    5: DeepBench.make5DIntArray,
}


def nested_list(dims, n):
    if dims == 1:
        return list(range(n))
    return [nested_list(dims - 1, n) for _ in range(n)]


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


print("=== pyjnius: list->array, multi-dimensional, push (Python -> Java) ===")
for dims in DIMS:
    size = 10 ** dims
    lst = nested_list(dims, 10)
    sumfn = SUM_BY_DIMS[dims]
    run(f"list->array int{'[]' * dims}(10^{dims}), fresh",
        lambda lst=lst, sumfn=sumfn: sumfn(lst), size)

print("=== pyjnius: array->list, multi-dimensional, pull (Java -> Python) ===")
for dims in DIMS:
    size = 10 ** dims
    makefn = MAKE_BY_DIMS[dims]
    run(f"array->list int{'[]' * dims}(10^{dims})",
        lambda makefn=makefn: makefn(10), size)

print("=== pyjnius: array->buffer, multi-dimensional, pull (Java -> Python) ===")
for dims in DIMS:
    size = 10 ** dims
    makefn = MAKE_BY_DIMS[dims]
    run(f"array->buffer int{'[]' * dims}(10^{dims})",
        lambda makefn=makefn: np.asarray(makefn(10)), size)
