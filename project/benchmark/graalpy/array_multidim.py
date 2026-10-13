"""Multi-dimensional array conversion, GraalPy side, 2D through 5D, holding
total element count fixed at each depth (10**dims elements). Swept across
four primitive element types (int32, int64, float32, float64). See
../array_flat.py (this file's counterpart, sweeping size instead of
nesting depth) and ../README.md.

GraalPy has no buffer->array push at all, at any depth (see
../array_flat.py's docstring for the confirmed TypeError and why the
missing path is emulated rather than skipped). Here that means:
  - "list->array": GraalPy's one automatic push path, unchanged.
  - "buffer->array (manual)": ../_arrayutil.py's build_manual() recurses
    once per dimension, allocating each level's Java array
    (java.type('<prim>[]...[]')) and assigning either a leaf scalar
    (dims==1) or a fully-built sub-array (dims>1) into each slot -- a
    genuine, if slow, per-element/per-row Python-level construction of
    the target array, all the way from a numpy source. There is no
    "list->array via np.array()" middle row here the way jpype's has:
    that row exists there to test whether routing a list through numpy
    first lets it land on a *fast* buffer path -- since GraalPy's buffer
    path is itself the slow manual one, not a bulk-copy fast path,
    routing through it first would just add numpy-array-construction
    cost on top of the same manual assembly, not reveal anything new.

Two categories in the pull section (GraalPy has no toList()-equivalent,
see ../array_flat.py):
  - pull, "array->list": a fully-materialized nested Python list,
    recursing over the returned `polyglot.ForeignList` one dimension at a
    time.
  - pull, "array->buffer": np.asarray(...) on the same return value.

The manual push category uses ../_arrayutil.py's calls_for_manual(), a
much smaller iteration budget than the other categories' calls_for() --
build_manual() pays one polyglot host-call per element, not per array,
and that per-crossing cost is orders of magnitude past a bulk-copy call
(see calls_for_manual()'s docstring), so this file can take several
minutes to run in full, particularly at 4D/5D where a 10,000-element
array is being assembled 10x10x10x10x10-nested, one leaf/row host-call
at a time.

Writes project/benchmark/graalpy/array_multidim_results.csv alongside the
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

DIMS = [2, 3, 4, 5]

# (label, numpy dtype, {sum2D..sum5D}, {make2D..make5D})
TYPES = [
    ('int', np.dtype('int32'), {
        2: DeepBench.sum2DIntArray, 3: DeepBench.sum3DIntArray,
        4: DeepBench.sum4DIntArray, 5: DeepBench.sum5DIntArray,
    }, {
        2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
        4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
    }),
    ('long', np.dtype('int64'), {
        2: DeepBench.sum2DLongArray, 3: DeepBench.sum3DLongArray,
        4: DeepBench.sum4DLongArray, 5: DeepBench.sum5DLongArray,
    }, {
        2: DeepBench.make2DLongArray, 3: DeepBench.make3DLongArray,
        4: DeepBench.make4DLongArray, 5: DeepBench.make5DLongArray,
    }),
    ('float', np.dtype('float32'), {
        2: DeepBench.sum2DFloatArray, 3: DeepBench.sum3DFloatArray,
        4: DeepBench.sum4DFloatArray, 5: DeepBench.sum5DFloatArray,
    }, {
        2: DeepBench.make2DFloatArray, 3: DeepBench.make3DFloatArray,
        4: DeepBench.make4DFloatArray, 5: DeepBench.make5DFloatArray,
    }),
    ('double', np.dtype('float64'), {
        2: DeepBench.sum2DDoubleArray, 3: DeepBench.sum3DDoubleArray,
        4: DeepBench.sum4DDoubleArray, 5: DeepBench.sum5DDoubleArray,
    }, {
        2: DeepBench.make2DDoubleArray, 3: DeepBench.make3DDoubleArray,
        4: DeepBench.make4DDoubleArray, 5: DeepBench.make5DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_multidim_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list(dims, n, leaf=int):
    if dims == 1:
        return [leaf(i) for i in range(n)]
    return [nested_list(dims - 1, n, leaf) for _ in range(n)]


def to_nested_list(ja, dims):
    """Fully materialize a GraalPy ForeignList array into plain (recursive)
    Python lists, for a fair comparison against array->buffer."""
    if dims == 1:
        return list(ja)
    return [to_nested_list(row, dims - 1) for row in ja]


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements, direction, source, dtype, dims, manual=False):
    n, warmup = calls_for_manual(total_elements) if manual else calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_multidim', direction=direction, source=source,
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


for label, dtype, SUM_BY_DIMS, MAKE_BY_DIMS in TYPES:
    leaf = float if label in ('float', 'double') else int

    print(f"=== GraalPy: list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        lst = nested_list(dims, 10, leaf)
        sumfn = SUM_BY_DIMS[dims]
        run(f"list->array {label}{'[]' * dims}(10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label, dims)

    print(f"=== GraalPy: buffer->array (manual), multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
        sumfn = SUM_BY_DIMS[dims]
        run(f"buffer->array {label}{'[]' * dims}(10^{dims}), manual",
            lambda arr=arr, sumfn=sumfn, label=label, dims=dims:
                sumfn(build_manual(arr, dims, label)), size,
            'push', 'buffer_manual', label, dims, manual=True)

    print(f"=== GraalPy: array->list, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->list {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size,
            'pull', 'list', label, dims)

    print(f"=== GraalPy: array->buffer, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->buffer {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: np.asarray(makefn(10)), size,
            'pull', 'buffer', label, dims)

csv_log.close()
