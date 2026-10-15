"""Ragged (jagged) nested-list push, GraalPy side, 2D through 5D. Ports
directly with no caveat, for the same reason as jep's/pyjnius's (see
../README.md): this is a push-side, plain-nested-list-only concept, and
GraalPy's list->array push (the only push path it has, see
../array_flat.py) is a per-element/per-row walk with no
ragged-vs-rectangular fast-path distinction to even ask a question about.
Companion to array_multidim.py; same DeepBench.sum{2,3,4,5}D{Type}Array
push entry point, but built from a tree whose branching factor varies at
every level (fixed seed, so repeated runs build identical trees) instead
of a uniform 10-wide rectangular one.

Writes project/benchmark/graalpy/array_ragged_results.csv alongside the
printed output.

Usage: run via the Bench launcher with DeepBench on the classpath (see
../README.md).
"""
import sys
import os
import random

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import java

DeepBench = java.type('jpype.benchmark.DeepBench')

DIMS = [2, 3, 4, 5]

TYPES = [
    ('int', {
        2: DeepBench.sum2DIntArray, 3: DeepBench.sum3DIntArray,
        4: DeepBench.sum4DIntArray, 5: DeepBench.sum5DIntArray,
    }),
    ('long', {
        2: DeepBench.sum2DLongArray, 3: DeepBench.sum3DLongArray,
        4: DeepBench.sum4DLongArray, 5: DeepBench.sum5DLongArray,
    }),
    ('float', {
        2: DeepBench.sum2DFloatArray, 3: DeepBench.sum3DFloatArray,
        4: DeepBench.sum4DFloatArray, 5: DeepBench.sum5DFloatArray,
    }),
    ('double', {
        2: DeepBench.sum2DDoubleArray, 3: DeepBench.sum3DDoubleArray,
        4: DeepBench.sum4DDoubleArray, 5: DeepBench.sum5DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_ragged_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list_ragged(dims, avg_n, seed=0, leaf=int):
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
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements, dtype, dims):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_ragged', direction='push', source='list',
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


for label, SUM_BY_DIMS in TYPES:
    leaf = float if label in ('float', 'double') else int
    print(f"=== GraalPy: ragged list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        lst = nested_list_ragged(dims, 10, seed=dims, leaf=leaf)
        size = count_elements(lst, dims)
        sumfn = SUM_BY_DIMS[dims]
        print(f"  (dims={dims}, actual element count={size})")
        run(f"ragged list->array {label}{'[]' * dims}(~10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size, label, dims)

csv_log.close()
