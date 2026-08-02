"""Flat (1D) array conversion, jpy side, at increasing sizes. Companion:
jpype/array_flat.py, jep/array_flat.py -- same operations and sizes, using
the shared jpype.benchmark.DeepBench test class. See ../array_multidim.py
and ../README.md.

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
1D).

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/array_flat.py \
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

SIZES = [100, 1_000, 10_000, 100_000]


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


print("=== jpy: list->array, flat, push (Python -> Java) ===")
for size in SIZES:
    lst = list(range(size))
    run(f"list->array int[{size}], fresh",
        lambda lst=lst: DeepBench.sumIntArray(lst), size)

print("=== jpy: buffer->array, flat, push (Python -> Java) ===")
for size in SIZES:
    arr = np.arange(size, dtype=np.int32)
    run(f"buffer->array int[{size}], fresh",
        lambda arr=arr: DeepBench.sumIntArray(arr), size)

print("=== jpy: array->list, flat, pull (Java -> Python) ===")
for size in SIZES:
    run(f"array->list int[{size}]",
        lambda size=size: list(DeepBench.makeIntArray(size)), size)

print("=== jpy: array->buffer, flat, pull (Java -> Python) ===")
for size in SIZES:
    run(f"array->buffer int[{size}]",
        lambda size=size: np.asarray(DeepBench.makeIntArray(size)), size)
