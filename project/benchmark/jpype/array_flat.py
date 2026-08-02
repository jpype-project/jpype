"""Flat (1D) array conversion, JPype side, at increasing sizes: 100, 1k,
10k, 100k elements -- how conversion cost scales with size. Companion:
jpy/array_flat.py, jep/array_flat.py -- same operations and sizes, using
the shared jpype.benchmark.DeepBench test class. See ../array_multidim.py
(this file's counterpart, sweeping nesting depth instead of size) and
../README.md.

Two categories per direction, not one "arrays" bucket -- a plain Python
list and a buffer-protocol object (numpy) hit genuinely different native
code paths, not just different inputs to the same one:
  - push (Python -> Java), "list->array": DeepBench.sumIntArray(list) --
    JPConversionSequence, a per-element walk (fast-pathed for homogeneous
    exact `int`s, but still one Python-level item lookup per element).
  - push (Python -> Java), "buffer->array": DeepBench.sumIntArray(numpy)
    -- JPConversionBuffer, a direct memory copy via the buffer protocol,
    no per-element Python-level access at all.
  - pull (Java -> Python), "array->list": list(DeepBench.makeIntArray(n))
    -- generic Python sequence iteration over the returned jpype array.
  - pull (Java -> Python), "array->buffer": np.asarray(...) on the same
    return value -- PyJPArrayPrimitive_getBuffer, a buffer-protocol read.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/array_flat.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import numpy as np
import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]


def calls_for(total_elements):
    """Scales iteration count down as array size grows so total elements
    moved per benchmark stays roughly bounded -- a naive fixed n=200_000
    would take minutes at 100k elements."""
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


print("=== JPype: list->array, flat, push (Python -> Java) ===")
for size in SIZES:
    lst = list(range(size))
    run(f"list->array int[{size}], fresh",
        lambda lst=lst: DeepBench.sumIntArray(lst), size)

print("=== JPype: buffer->array, flat, push (Python -> Java) ===")
for size in SIZES:
    arr = np.arange(size, dtype=np.int32)
    run(f"buffer->array int[{size}], fresh",
        lambda arr=arr: DeepBench.sumIntArray(arr), size)

print("=== JPype: array->list, flat, pull (Java -> Python) ===")
for size in SIZES:
    run(f"array->list int[{size}]",
        lambda size=size: list(DeepBench.makeIntArray(size)), size)

print("=== JPype: array->buffer, flat, pull (Java -> Python) ===")
for size in SIZES:
    run(f"array->buffer int[{size}]",
        lambda size=size: np.asarray(DeepBench.makeIntArray(size)), size)

jpype.shutdownJVM()
