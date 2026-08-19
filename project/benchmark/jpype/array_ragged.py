"""Ragged (jagged) nested-list push, JPype side, 2D through 5D --
specifically the case array_multidim.py's `nested_list()` never
exercises (every sibling length there is fixed at 10, so a genuinely
irregular tree has no baseline there). Companion to array_multidim.py;
same DeepBench.void{2,3,4,5}D{Type}Array push entry point, but built from a
tree whose branching factor varies at every level (fixed seed, so
"before" and "after" runs against the same commit produce identical
trees and therefore a fair per-call comparison) instead of a uniform
10-wide rectangular one. Swept across the four primitive element types
already ragged-eligible before this addition (int32, int64, float32,
float64, as in array_flat.py/array_multidim.py) plus the four narrower
leaf types isRaggedEligible (jp_classhints.cpp) grew to cover in the same
change that added the padding this file's byte/boolean/char/short rows
exercise: 1-byte-wide byte/boolean and 2-byte-wide char/short, whose leaf
runs need 4-byte padding after encoding (raggedAlign4) that the 4-/8-byte
int/long/float/double rows never pay -- this is the isRaggedEligible-vs-
isRaggedEligible axis, not a numpy-dtype one, so unlike array_of.py's
four-way dtype comparison there's no "auto"/"matching"/"cross"/"naive"
split here, just eight leaf types over the same ragged-push shape.

Writes project/benchmark/jpype/array_ragged_results.csv alongside the
printed output.
"""
import sys
import os
import random

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

DIMS = [2, 3, 4, 5]

TYPES = [
    ('int', {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }),
    ('long', {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }),
    ('float', {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }),
    ('double', {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
        4: DeepBench.void4DDoubleArray, 5: DeepBench.void5DDoubleArray,
    }),
    ('byte', {
        2: DeepBench.void2DByteArray, 3: DeepBench.void3DByteArray,
        4: DeepBench.void4DByteArray, 5: DeepBench.void5DByteArray,
    }),
    ('boolean', {
        2: DeepBench.void2DBooleanArray, 3: DeepBench.void3DBooleanArray,
        4: DeepBench.void4DBooleanArray, 5: DeepBench.void5DBooleanArray,
    }),
    ('char', {
        2: DeepBench.void2DCharArray, 3: DeepBench.void3DCharArray,
        4: DeepBench.void4DCharArray, 5: DeepBench.void5DCharArray,
    }),
    ('short', {
        2: DeepBench.void2DShortArray, 3: DeepBench.void3DShortArray,
        4: DeepBench.void4DShortArray, 5: DeepBench.void5DShortArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_ragged_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list_ragged(dims, avg_n, seed=0, leaf=int):
    """Sibling lengths vary uniformly in [avg_n-4, avg_n+4] at every
    level (including the leaf level) -- genuinely ragged at every depth,
    not just the outermost. Fixed seed so repeated runs (e.g. before vs.
    after a code change) build the exact same tree.

    leaf must produce exactly the Python type isRaggedLeafElement
    (jp_classhints.cpp) requires for the target leaf type, or the
    ragged-native fast path silently declines and falls back to the
    general per-element path instead -- PyFloat_CheckExact for F/D,
    PyLong_CheckExact for I/J/B/S, PyBool_Check for Z, and an exact
    length-1 str for C. A plain Python int is valid for a float[]/
    double[]/boolean[]/char[] target too (Java widens/truthies/indexes
    it), just not via this fast path."""
    rng = random.Random(seed)

    def build(d):
        n = rng.randint(max(1, avg_n - 4), avg_n + 4)
        if d == 1:
            return [leaf(i) for i in range(n)]
        return [build(d - 1) for _ in range(n)]
    return build(dims)


def count_elements(node, dims):
    if dims == 1:
        return len(node)
    return sum(count_elements(child, dims - 1) for child in node)


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, dtype, dims):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_ragged', direction='push', source='list',
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


LEAF_BY_LABEL = {
    'int': int, 'long': int, 'float': float, 'double': float,
    'byte': int, 'short': int,
    'boolean': lambda i: bool(i % 2),
    'char': lambda i: chr(ord('a') + i % 26),
}

for label, SUM_BY_DIMS in TYPES:
    leaf = LEAF_BY_LABEL[label]
    print(f"=== JPype: ragged list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        lst = nested_list_ragged(dims, 10, seed=dims, leaf=leaf)
        size = count_elements(lst, dims)
        sumfn = SUM_BY_DIMS[dims]
        print(f"  (dims={dims}, actual element count={size})")
        run(f"ragged list->array {label}{'[]' * dims}(~10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size, label, dims)

csv_log.close()
jpype.shutdownJVM()
