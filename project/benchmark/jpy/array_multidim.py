"""Multi-dimensional array conversion, jpy side, 2D through 5D, holding
total element count fixed at each depth (matching array_flat.py's sizes).
Swept across four primitive element types (int32, int64, float32,
float64). Companion: jpype/array_multidim.py, jep/array_multidim.py --
same operations and depths, using the shared jpype.benchmark.DeepBench
test class. See ../array_flat.py and ../README.md.

Two categories per direction, not one "arrays" bucket -- see
../array_flat.py for why list vs. buffer input matter as a distinction at
all. For multi-dimensional arrays specifically, jpy has no bulk buffer
path in *either* direction (confirmed from source, not inferred from
timing), so both categories collapse onto the same generic per-element
code path here -- kept as separate rows anyway for a direct comparison
against jpype's (which does have a real fast path) and jep's (partial)
rows of the same name:
  - push, "list->array"/"buffer->array": DeepBench.void{2,3,4,5}D{Type}Array
    on a nested list or a numpy array respectively -- jpy's general
    per-element recursion (`JType_CreateJavaArray`, jpy_jtype.c) handles
    a numpy sub-array through the exact same code path as any other
    Python sequence (no numpy-specific branch exists for anything but a
    flat 1D target -- see array_flat.py), but that does *not* mean
    near-identical cost between the two rows -- measured (RESULTS.md
    Section 5), `buffer->array` here is consistently 2.65-3.1x *slower*
    than `list->array`, not close to it. Same code path, different
    underlying object costs: `PySequence_GetItem` on a numpy sub-array
    falls through to `mp_subscript` (numpy's own `__getitem__`), which
    builds a new ndarray view object per row instead of a list's direct
    `sq_item` slot read, and leaf-level conversion
    (`JPy_AS_JINT`/`PyLong_AsLong`) receives a `numpy.int32` scalar
    rather than a native Python `int`, routing through numpy's
    `__index__` protocol instead of CPython's native-int fast path --
    both add real per-call cost on top of the shared recursion, and the
    leaf-level one dominates since leaf calls (10^depth) outnumber
    row-level calls at every depth. This is still true type-by-type,
    not just for int: nothing in jpy's per-element recursion is
    type-specific either, so the same two costs apply uniformly.
  - pull, "array->list"/"array->buffer": jpy only registers a
    getbufferproc for 1D primitive-leaf array types (jpy_jobj.c:
    tp_as_buffer only set when isPrimitiveArray) -- an int[][]-and-deeper
    jpy array has no buffer protocol at all, so np.asarray() falls back
    to the same generic per-element sequence walk (__len__/__getitem__)
    that building a plain nested list does. Expect near-identical cost
    here too.

Writes project/benchmark/jpy/array_multidim_results.csv alongside the
printed output.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/array_multidim.py \
        /path/to/jpype/test/classes /path/to/jpype/test/harness
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import numpy as np
import jpyutil

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'

jpyutil.init_jvm(jvm_maxmem='512M', jvm_classpath=[classes_dir, harness_dir])
import jpy

DeepBench = jpy.get_type('jpype.benchmark.DeepBench')

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
    """leaf converts each leaf value. jpy's per-element recursion
    (JType_CreateJavaArray, jpy_jtype.c) has no exact-type fast path to
    miss the way jpype's ragged-native list push does -- a plain int
    leaf for a float[][] target still goes through the same generic
    per-element conversion either way -- but leaf is kept here anyway so
    the input data matches the jpype/jep versions of this file exactly,
    for a fair per-type comparison across libraries."""
    if dims == 1:
        return [leaf(i) for i in range(n)]
    return [nested_list(dims - 1, n, leaf) for _ in range(n)]


def to_nested_list(ja, dims):
    """Fully materialize a jpy multi-dimensional array into plain
    (recursive) Python lists, for a fair comparison against array->buffer
    -- a shallow list(ja) would only give a list of jpy sub-array
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

    print(f"=== jpy: list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        lst = nested_list(dims, 10, leaf)
        sumfn = SUM_BY_DIMS[dims]
        run(f"list->array {label}{'[]' * dims}(10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label, dims)

    print(f"=== jpy: buffer->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
        sumfn = SUM_BY_DIMS[dims]
        run(f"buffer->array {label}{'[]' * dims}(10^{dims}), fresh",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), size,
            'push', 'buffer', label, dims)

    print(f"=== jpy: array->list, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->list {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size,
            'pull', 'list', label, dims)

    print(f"=== jpy: array->buffer, multi-dimensional, pull (Java -> Python), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->buffer {label}{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: np.asarray(makefn(10)), size,
            'pull', 'buffer', label, dims)

csv_log.close()
