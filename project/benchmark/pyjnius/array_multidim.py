"""Multi-dimensional array conversion, pyjnius side, 2D through 5D,
holding total element count fixed at each depth (matching
array_flat.py's sizes). Swept across four primitive element types
(int32, int64, float32, float64), same sweep as jpype/array_multidim.py.
Companion: jpype/array_multidim.py, jpy/array_multidim.py,
jep/array_multidim.py -- same operations and depths, using the shared
jpype.benchmark.DeepBench test class. See ../array_flat.py and
../README.md.

Same story as array_flat.py, confirmed at every depth: pyjnius rejects
numpy input for array arguments unconditionally (`JavaException(
'Expecting a python list/tuple, got array(...)')`), so there's no
buffer->array push row here either -- only "list->array" (nested Python
lists, which pyjnius's general per-element conversion does accept and
recurse through correctly). And every pull direction returns an already
fully-materialized, recursively nested plain Python list -- confirmed
for 2D (`type(DeepBench.make2DIntArray(4))` is `list`, and so is
`type(DeepBench.make2DIntArray(4)[0])`) -- so "array->list" is again
just the raw return value, with no separate materialization step to
benchmark (unlike jpype's array_multidim.py, whose `to_nested_list`
helper has real work to do walking a jpype array object one dimension
at a time -- there is no such object here to walk), and "array->buffer"
is that same value with an extra `np.asarray()` step on top, never
faster.

Writes project/benchmark/pyjnius/array_multidim_results.csv alongside
the printed output.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/array_multidim.py \
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

DIMS = [2, 3, 4, 5]

# (label, {sum2D..sum5D}, {make2D..make5D})
TYPES = [
    ('int', {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }, {
        2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
        4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
    }),
    ('long', {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }, {
        2: DeepBench.make2DLongArray, 3: DeepBench.make3DLongArray,
        4: DeepBench.make4DLongArray, 5: DeepBench.make5DLongArray,
    }),
    ('float', {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }, {
        2: DeepBench.make2DFloatArray, 3: DeepBench.make3DFloatArray,
        4: DeepBench.make4DFloatArray, 5: DeepBench.make5DFloatArray,
    }),
    ('double', {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
        4: DeepBench.void4DDoubleArray, 5: DeepBench.void5DDoubleArray,
    }, {
        2: DeepBench.make2DDoubleArray, 3: DeepBench.make3DDoubleArray,
        4: DeepBench.make4DDoubleArray, 5: DeepBench.make5DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_multidim_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list(dims, n, leaf=int):
    """leaf must produce a genuine (exact-type) Python float for a
    float[]/double[] target -- pyjnius's per-element conversion path
    widens a plain Python int to a Java float/double just fine, but this
    mirrors jpype's array_multidim.py leaf-type discipline so the two
    side-by-side CSVs describe the exact same input shapes per type."""
    if dims == 1:
        return [leaf(i) for i in range(n)]
    return [nested_list(dims - 1, n, leaf) for _ in range(n)]


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, direction, source, dtype, dims):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_multidim', direction=direction, source=source,
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


for label, SUM_BY_DIMS, MAKE_BY_DIMS in TYPES:
    leaf = float if label in ('float', 'double') else int

    print(f"=== pyjnius: list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        lst = nested_list(dims, 10, leaf)
        sumfn = SUM_BY_DIMS[dims]
        run(f"list->array {label}{'[]' * dims}(10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label, dims)

    print(f"=== pyjnius: array->list, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->list {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: makefn(10), size,
            'pull', 'list', label, dims)

    print(f"=== pyjnius: array->buffer, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->buffer {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: np.asarray(makefn(10)), size,
            'pull', 'buffer', label, dims)

csv_log.close()
