"""Flat (1D) array conversion, JPype side, at increasing sizes: 100, 1k,
10k, 100k elements -- how conversion cost scales with size. Swept across
four primitive element types (int32, int64, float32, float64) so a
per-type gap (not just a per-size one) shows up if there is one. Companion:
jpy/array_flat.py, jep/array_flat.py -- same operations and sizes (int
only), using the shared jpype.benchmark.DeepBench test class. See
../array_multidim.py (this file's counterpart, sweeping nesting depth
instead of size), ../array_shape.py (sweeping shape at fixed depth), and
../README.md.

Four categories per direction, not one "arrays" bucket -- a plain Python
list and a buffer-protocol object (numpy) hit genuinely different native
code paths, not just different inputs to the same one:
  - push (Python -> Java), "list->array": DeepBench.void{Type}Array(list) --
    JPConversionSequence, a per-element walk (fast-pathed for homogeneous
    exact Python numbers of the matching kind, but still one Python-level
    item lookup per element).
  - push (Python -> Java), "buffer->array": DeepBench.void{Type}Array(numpy)
    -- JPConversionBuffer, a direct memory copy via the buffer protocol,
    no per-element Python-level access at all.
  - pull (Java -> Python), "array->list": list(DeepBench.make{Type}Array(n))
    -- generic Python sequence iteration over the returned jpype array.
  - pull (Java -> Python), "array->buffer": np.asarray(...) on the same
    return value -- PyJPArrayPrimitive_getBuffer, a buffer-protocol read.

Writes project/benchmark/jpype/array_flat_results.csv alongside the
printed output.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/array_flat.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import numpy as np
import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]

# (label, numpy dtype, sum{Type}Array, make{Type}Array)
TYPES = [
    ('int', np.dtype('int32'), DeepBench.voidIntArray, DeepBench.makeIntArray),
    ('long', np.dtype('int64'), DeepBench.voidLongArray, DeepBench.makeLongArray),
    ('float', np.dtype('float32'), DeepBench.voidFloatArray, DeepBench.makeFloatArray),
    ('double', np.dtype('float64'), DeepBench.voidDoubleArray, DeepBench.makeDoubleArray),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_flat_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    """Scales iteration count down as array size grows so total elements
    moved per benchmark stays roughly bounded -- a naive fixed n=200_000
    would take minutes at 100k elements."""
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, direction, source, dtype):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_flat', direction=direction, source=source,
                   dtype=dtype, size=total_elements, n=n, best_ns=best, median_ns=median)


for label, dtype, sumfn, makefn in TYPES:
    print(f"=== JPype: list->array, flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        # Python-level element type matches the target array's own kind
        # (float list for float[]/double[], int list for int[]/long[]) --
        # exercises each type's own homogeneous fast path, not a widening
        # conversion from a different Python type.
        lst = [float(i) for i in range(size)] if dtype.kind == 'f' else list(range(size))
        run(f"list->array {label}[{size}], fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label)

    if dtype.kind == 'f':
        print(f"=== JPype: list->array, flat, push (Python -> Java), {label}, widening from int ===")
        for size in SIZES:
            # A plain Python int list pushed into a float[]/double[]
            # target -- idiomatic (`javaMethod([1, 2, 3])` against a
            # double[] parameter), and NOT the same benchmark as the
            # homogeneous-type row above: int list elements don't hit
            # PyFloat_CheckExact, so this never takes the fast
            # PyList_CheckExact loop and always falls to the general
            # per-element path.
            lst = list(range(size))
            run(f"list->array {label}[{size}], widening from int",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
                'push', 'list_widen', label)

    print(f"=== JPype: buffer->array, flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        arr = np.arange(size, dtype=dtype)
        run(f"buffer->array {label}[{size}], fresh",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), size,
            'push', 'buffer', label)

    print(f"=== JPype: array->list, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->list {label}[{size}]",
            lambda size=size, makefn=makefn: list(makefn(size)), size,
            'pull', 'list', label)

    print(f"=== JPype: array->list via toList(), flat, pull (Java -> Python), {label} ===")
    # toList(): one JNI critical section for the whole array instead of one
    # JNI call per element via list()'s native sq_item iteration -- same
    # output, compare directly against the row above.
    for size in SIZES:
        run(f"array->list.toList() {label}[{size}]",
            lambda size=size, makefn=makefn: makefn(size).toList(), size,
            'pull', 'toList', label)

    print(f"=== JPype: array->buffer, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->buffer {label}[{size}]",
            lambda size=size, makefn=makefn: np.asarray(makefn(size)), size,
            'pull', 'buffer', label)

csv_log.close()
jpype.shutdownJVM()
