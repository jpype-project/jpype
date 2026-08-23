"""Flat (1D) array conversion, jpy side, at increasing sizes. Swept across
four primitive element types (int32, int64, float32, float64) so a
per-type gap (not just a per-size one) shows up if there is one.
Companion: jpype/array_flat.py, jep/array_flat.py -- same operations and
sizes, using the shared jpype.benchmark.DeepBench test class. See
../array_multidim.py and ../README.md.

Two categories per direction, not one "arrays" bucket -- a plain Python
list and a buffer-protocol object (numpy) are genuinely different code
paths in jpy too, not just different inputs to the same one: "push,
list->array" is a per-element `PySequence_GetItem` loop
(`JType_CreateJavaArray`, jpy_jtype.c) regardless of what's being
iterated, while "push, buffer->array" -- passing a numpy array as a
method argument, not via `jpy.array()` (see ../README.md) -- takes a
separate `PyObject_CheckBuffer` branch in jpy's own argument-matching
code (jpy_jtype.c). On the pull side, "array->list" is generic Python
sequence iteration over the returned jpy array wrapper, while
"array->buffer" hits a real registered `getbufferproc` (1D primitive-leaf
jpy arrays only -- see ../array_multidim.py for why that's not true past
1D). jpy has no `toList()`-equivalent fast path (that's a jpype-only
API), so this file stays at four rows per type, not five.

Writes project/benchmark/jpy/array_flat_results.csv alongside the
printed output.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/array_flat.py \
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
    print(f"=== jpy: list->array, flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        # Python-level element type matches the target array's own kind --
        # exercises each type's own homogeneous push, not a widening
        # conversion from a different Python type.
        lst = [float(i) for i in range(size)] if dtype.kind == 'f' else list(range(size))
        run(f"list->array {label}[{size}], fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
            'push', 'list', label)

    if dtype.kind == 'f':
        print(f"=== jpy: list->array, flat, push (Python -> Java), {label}, widening from int ===")
        for size in SIZES:
            # A plain Python int list pushed into a float[]/double[]
            # target -- idiomatic, and not the same benchmark as the
            # homogeneous-type row above.
            lst = list(range(size))
            run(f"list->array {label}[{size}], widening from int",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
                'push', 'list_widen', label)

    print(f"=== jpy: buffer->array, flat, push (Python -> Java), {label} ===")
    for size in SIZES:
        arr = np.arange(size, dtype=dtype)
        run(f"buffer->array {label}[{size}], fresh",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), size,
            'push', 'buffer', label)

    print(f"=== jpy: array->list, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->list {label}[{size}]",
            lambda size=size, makefn=makefn: list(makefn(size)), size,
            'pull', 'list', label)

    print(f"=== jpy: array->buffer, flat, pull (Java -> Python), {label} ===")
    for size in SIZES:
        run(f"array->buffer {label}[{size}]",
            lambda size=size, makefn=makefn: np.asarray(makefn(size)), size,
            'pull', 'buffer', label)

csv_log.close()
