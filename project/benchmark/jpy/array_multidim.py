"""Multi-dimensional array conversion, jpy side, 2D through 5D, holding
total element count fixed at each depth (matching array_flat.py's sizes).
Companion: jpype/array_multidim.py, jep/array_multidim.py -- same
operations and depths, using the shared jpype.benchmark.DeepBench test
class. See ../array_flat.py and ../README.md.

Two categories per direction, not one "arrays" bucket -- see
../array_flat.py for why list vs. buffer input matter as a distinction at
all. For multi-dimensional arrays specifically, jpy has no bulk buffer
path in *either* direction (confirmed from source, not inferred from
timing), so both categories collapse onto the same generic per-element
code path here -- kept as separate rows anyway for a direct comparison
against jpype's (which does have a real fast path) and jep's (partial)
rows of the same name:
  - push, "list->array"/"buffer->array": DeepBench.sum{2,3,4,5}DIntArray
    on a nested list or a numpy array respectively -- jpy's general
    per-element recursion (`JType_CreateJavaArray`, jpy_jtype.c) handles
    a numpy sub-array exactly like any other Python sequence, so expect
    near-identical cost between the two rows (no numpy-specific branch
    exists for anything but a flat 1D target -- see array_flat.py).
  - pull, "array->list"/"array->buffer": jpy only registers a
    getbufferproc for 1D primitive-leaf array types (jpy_jobj.c:
    tp_as_buffer only set when isPrimitiveArray) -- an int[][]-and-deeper
    jpy array has no buffer protocol at all, so np.asarray() falls back
    to the same generic per-element sequence walk (__len__/__getitem__)
    that building a plain nested list does. Expect near-identical cost
    here too.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/array_multidim.py \
        /path/to/jpype/test/classes /path/to/jpype/test/harness
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import numpy as np
import jpyutil

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'

jpyutil.init_jvm(jvm_maxmem='512M', jvm_classpath=[classes_dir, harness_dir])
import jpy

DeepBench = jpy.get_type('jpype.benchmark.DeepBench')

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


def to_nested_list(ja, dims):
    """Fully materialize a jpy multi-dimensional array into plain
    (recursive) Python lists, for a fair comparison against array->buffer
    -- a shallow list(ja) would only give a list of jpy sub-array
    objects, not plain ints, at any depth beyond 1."""
    if dims == 1:
        return list(ja)
    return [to_nested_list(row, dims - 1) for row in ja]


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


print("=== jpy: list->array, multi-dimensional, push (Python -> Java) ===")
for dims in DIMS:
    size = 10 ** dims
    lst = nested_list(dims, 10)
    sumfn = SUM_BY_DIMS[dims]
    run(f"list->array int{'[]' * dims}(10^{dims}), fresh",
        lambda lst=lst, sumfn=sumfn: sumfn(lst), size)

print("=== jpy: buffer->array, multi-dimensional, push (Python -> Java) ===")
for dims in DIMS:
    size = 10 ** dims
    arr = np.arange(size, dtype=np.int32).reshape((10,) * dims)
    sumfn = SUM_BY_DIMS[dims]
    run(f"buffer->array int{'[]' * dims}(10^{dims}), fresh",
        lambda arr=arr, sumfn=sumfn: sumfn(arr), size)

print("=== jpy: array->list, multi-dimensional, pull (Java -> Python) ===")
for dims in DIMS:
    size = 10 ** dims
    makefn = MAKE_BY_DIMS[dims]
    run(f"array->list int{'[]' * dims}(10^{dims})",
        lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size)

print("=== jpy: array->buffer, multi-dimensional, pull (Java -> Python) ===")
for dims in DIMS:
    size = 10 ** dims
    makefn = MAKE_BY_DIMS[dims]
    run(f"array->buffer int{'[]' * dims}(10^{dims})",
        lambda makefn=makefn: np.asarray(makefn(10)), size)
