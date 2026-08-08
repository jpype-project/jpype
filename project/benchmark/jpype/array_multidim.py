"""Multi-dimensional array conversion, JPype side, 2D through 5D, holding
total element count fixed at each depth (10**dims elements, so e.g. the 3D
case and array_flat.py's 10k case both move 10,000 ints) -- isolates
per-dimension nesting overhead from raw element count. Companion:
jpy/array_multidim.py, jep/array_multidim.py -- same operations and
depths, using the shared jpype.benchmark.DeepBench test class. See
../array_flat.py (this file's counterpart, sweeping size instead of
nesting depth) and ../README.md.

Two categories per direction, not one "arrays" bucket -- see
../array_flat.py for why list vs. buffer input are genuinely different
native code paths, not just different inputs to the same one:
  - push, "list->array": DeepBench.sum{2,3,4,5}DIntArray(nested_list) --
    JPConversionSequence recursing once per nesting level, materializing
    a fresh Python-level sub-sequence access at every row.
  - push, "buffer->array": DeepBench.sum{2,3,4,5}DIntArray(numpy_array)
    -- JPConversionMultiArrayBuffer, which fires when the buffer's ndim
    matches the target's nesting depth exactly: one bulk copy for the
    whole array, no per-row Python-level access at all.
  - pull, "array->list": a fully-materialized nested Python list of
    plain ints, built by recursing over the returned jpype array
    (there's no bulk path for this -- see below).
  - pull, "array->buffer": np.asarray(...) on the same return value --
    JPArray_getBuffer's collectRectangular, a bulk rectangular read.

DeepBench.make{2,3,4,5}DIntArray builds a fresh Java array and returns it
on every call for both pull rows.
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import numpy as np
import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

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
    """Fully materialize a jpype multi-dimensional array into plain
    (recursive) Python lists, for a fair comparison against array->buffer
    -- a shallow list(ja) would only give a list of jpype sub-array
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


print("=== JPype: list->array, multi-dimensional, push (Python -> Java) ===")
for dims in DIMS:
    size = 10 ** dims
    lst = nested_list(dims, 10)
    sumfn = SUM_BY_DIMS[dims]
    run(f"list->array int{'[]' * dims}(10^{dims}), fresh",
        lambda lst=lst, sumfn=sumfn: sumfn(lst), size)

print("=== JPype: buffer->array, multi-dimensional, push (Python -> Java) ===")
for dims in DIMS:
    size = 10 ** dims
    arr = np.arange(size, dtype=np.int32).reshape((10,) * dims)
    sumfn = SUM_BY_DIMS[dims]
    run(f"buffer->array int{'[]' * dims}(10^{dims}), fresh",
        lambda arr=arr, sumfn=sumfn: sumfn(arr), size)

print("=== JPype: array->list, multi-dimensional, pull (Java -> Python) ===")
for dims in DIMS:
    size = 10 ** dims
    makefn = MAKE_BY_DIMS[dims]
    run(f"array->list int{'[]' * dims}(10^{dims})",
        lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size)

print("=== JPype: array->buffer, multi-dimensional, pull (Java -> Python) ===")
for dims in DIMS:
    size = 10 ** dims
    makefn = MAKE_BY_DIMS[dims]
    run(f"array->buffer int{'[]' * dims}(10^{dims})",
        lambda makefn=makefn: np.asarray(makefn(10)), size)

jpype.shutdownJVM()
