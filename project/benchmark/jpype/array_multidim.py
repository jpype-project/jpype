"""Multi-dimensional array conversion, JPype side, 2D through 5D, holding
total element count fixed at each depth (10**dims elements, so e.g. the 3D
case and array_flat.py's 10k case both move 10,000 elements) -- isolates
per-dimension nesting overhead from raw element count. Swept across four
primitive element types (int32, int64, float32, float64). Companion:
jpy/array_multidim.py, jep/array_multidim.py -- same operations and
depths (int only), using the shared jpype.benchmark.DeepBench test class.
See ../array_flat.py (this file's counterpart, sweeping size instead of
nesting depth), ../array_shape.py (sweeping shape, e.g. 10000x10 vs.
10x10000, at a *fixed* depth and total -- this file always uses a
uniform, equal-length-per-dimension shape, so it can't show whether shape
itself matters independent of depth), and ../README.md.

Three categories in the push section, not two -- see ../array_flat.py for
why list vs. buffer input are genuinely different native code paths, not
just different inputs to the same one:
  - push, "list->array": DeepBench.void{2,3,4,5}D{Type}Array(nested_list) --
    JPConversionSequence recursing once per nesting level, materializing
    a fresh Python-level sub-sequence access at every row. The current,
    only path for a plain nested list today.
  - push, "list->array via np.array()": same nested_list input, but
    converted to a numpy array in Python first -- a stand-in for
    validating the "build one buffer, then bulk-push it" hypothesis
    before writing any new conversion code -- and pushed through the
    existing buffer->array row's fast path. Total cost = walking the
    nested list once to build the
    buffer (np.array's own job) + one bulk copy, vs. JPConversionSequence
    walking it once *and* paying a JNI array-build call per row as it
    goes. np.array() itself is not the proposed implementation (a real
    fix would build the buffer in C++, not delegate to numpy) -- it's a
    stand-in that's fast/correct enough to tell whether the general
    shape of the idea is worth pursuing at all.
  - push, "buffer->array": DeepBench.void{2,3,4,5}D{Type}Array(numpy_array)
    -- JPConversionMultiArrayBuffer, which fires when the buffer's ndim
    matches the target's nesting depth exactly: one bulk copy for the
    whole array, no per-row Python-level access at all. Also the second
    half of the "via np.array()" row above once the array is built --
    included on its own so the buffer-only cost is visible separately
    from the list-walk cost that precedes it there.

Three categories in the pull section:
  - pull, "array->list": a fully-materialized nested Python list of
    plain values, built by recursing over the returned jpype array one
    dimension at a time via plain Python iteration/`list()`.
  - pull, "array->list via toList()": same output, but through
    JArray.toList() -- one JNI critical section per leaf array instead
    of one JNI call per element, compare directly against the row above.
  - pull, "array->buffer": np.asarray(...) on the same return value --
    JPArray_getBuffer's collectRectangular, a bulk rectangular read.

DeepBench.make{2,3,4,5}D{Type}Array builds a fresh Java array and returns
it on every call for both pull rows.

Writes project/benchmark/jpype/array_multidim_results.csv alongside the
printed output.
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import numpy as np
import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

DIMS = [2, 3, 4, 5]

# (label, numpy dtype, {sum2D..sum5D}, {make2D..make5D})
TYPES = [
    ('int', np.dtype('int32'), {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }, {
        2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
        4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
    }),
    ('long', np.dtype('int64'), {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }, {
        2: DeepBench.make2DLongArray, 3: DeepBench.make3DLongArray,
        4: DeepBench.make4DLongArray, 5: DeepBench.make5DLongArray,
    }),
    ('float', np.dtype('float32'), {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }, {
        2: DeepBench.make2DFloatArray, 3: DeepBench.make3DFloatArray,
        4: DeepBench.make4DFloatArray, 5: DeepBench.make5DFloatArray,
    }),
    ('double', np.dtype('float64'), {
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
    """leaf converts each leaf value -- must be a genuine (exact-type)
    Python float for a float[]/double[] target, not just any numeric
    value: the ragged-native list-push fast path's leaf check
    (isRaggedLeafElement, jp_classhints.cpp) requires PyFloat_CheckExact
    for F/D leaves, same as it requires PyLong_CheckExact for I/J -- a
    plain Python int handed to a float[][] target is valid (Java widens
    it), but misses this fast path entirely and silently falls back to
    the general per-element conversion instead, which is not what this
    benchmark is trying to measure here."""
    if dims == 1:
        return [leaf(i) for i in range(n)]
    return [nested_list(dims - 1, n, leaf) for _ in range(n)]


def to_nested_list(ja, dims):
    """Fully materialize a jpype multi-dimensional array into plain
    (recursive) Python lists, for a fair comparison against array->buffer
    -- a shallow list(ja) would only give a list of jpype sub-array
    objects, not plain values, at any depth beyond 1."""
    if dims == 1:
        return list(ja)
    return [to_nested_list(row, dims - 1) for row in ja]


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


for label, dtype, SUM_BY_DIMS, MAKE_BY_DIMS in TYPES:
    leaf = float if label in ('float', 'double') else int

    print(f"=== JPype: list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        lst = nested_list(dims, 10, leaf)
        sumfn = SUM_BY_DIMS[dims]
        run(f"list->array {label}{'[]' * dims}(10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label, dims)

    print(f"=== JPype: list->array via np.array(), multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        lst = nested_list(dims, 10, leaf)
        sumfn = SUM_BY_DIMS[dims]

        def via_numpy(lst=lst, sumfn=sumfn, dtype=dtype):
            return sumfn(np.array(lst, dtype=dtype))
        run(f"list->array(np.array()) {label}{'[]' * dims}(10^{dims}), fresh",
            via_numpy, size, 'push', 'list_via_numpy', label, dims)

    print(f"=== JPype: buffer->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
        sumfn = SUM_BY_DIMS[dims]
        run(f"buffer->array {label}{'[]' * dims}(10^{dims}), fresh",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), size,
            'push', 'buffer', label, dims)

    print(f"=== JPype: array->list, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->list {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size,
            'pull', 'list', label, dims)

    print(f"=== JPype: array->list via toList(), multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->list.toList() {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: makefn(10).toList(), size,
            'pull', 'toList', label, dims)

    print(f"=== JPype: array->buffer, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->buffer {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: np.asarray(makefn(10)), size,
            'pull', 'buffer', label, dims)

csv_log.close()
jpype.shutdownJVM()
