"""Ragged (jagged) nested-list push, jep side, 2D through 5D --
specifically the case array_multidim.py's `nested_list()` never
exercises (every sibling length there is fixed at 10, so a genuinely
irregular tree has no baseline there). Companion to jpype/array_ragged.py,
jpy/array_ragged.py; same DeepBench.void{2,3,4,5}D{Type}Array push entry
point, but built from a tree whose branching factor varies at every level
(fixed seed, so "before" and "after" runs against the same commit produce
identical trees and therefore a fair per-call comparison) instead of a
uniform 10-wide rectangular one. Swept across the same four primitive
element types (int32, int64, float32, float64) as array_flat.py/
array_multidim.py.

jpype has a ragged-native list-push fast path (isRaggedLeafElement,
jp_classhints.cpp) that specifically distinguishes rectangular from
irregular nested-list shapes -- this file exists on the jpype side to
measure whether that fast path still applies to a genuinely ragged tree
(it does, by design). jep has no equivalent shape-specialized fast path
to test either way: `pyfastsequence_as_jobject`'s per-element recursion
(see array_flat.py/array_multidim.py's module docstrings) doesn't
validate or special-case sibling-length uniformity up front at any
level -- it just walks whatever sequence it's handed, ragged or not, one
`PySequence_Fast_GET_ITEM` at a time. So there's no "does jep have a
ragged fast path" question to answer the way there is for jpype; what
this benchmark actually shows is whether jep's plain per-element
recursion costs the same for a ragged tree as array_multidim.py's
rectangular one did at a matching element count -- expected to come out
equivalent, since the push path never inspects shape before walking it,
but that's what the recorded numbers should confirm, not something
assumed here.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.
For the same reason, _common.CsvLog can't be imported either -- see the
inlined CsvLog class below.

Writes a CSV (fieldnames: category, direction, source, dtype, dims,
size, n, best_ns, median_ns) to the path given as argv[2], defaulting to
"array_ragged_results.csv" next to out_path.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_ragged.py <output_path> [<csv_path>]
"""
import sys
import os
import csv
import time
import random


def timeit(fn, n=200_000, warmup=1000, trials=7):
    for _ in range(warmup):
        fn()
    samples = []
    for _ in range(trials):
        t0 = time.perf_counter()
        for _ in range(n):
            fn()
        t1 = time.perf_counter()
        samples.append((t1 - t0) / n * 1e9)
    samples.sort()
    return samples[0], samples[len(samples) // 2]


def format_row(name, best, median):
    return f"{name:32s} best={best:8.1f} ns/call  median={median:8.1f} ns/call"


class CsvLog:
    """Inlined equivalent of _common.CsvLog -- see array_flat.py for why
    this can't just be imported here."""

    def __init__(self, path, fieldnames):
        self._fieldnames = fieldnames
        self._f = open(path, 'w', newline='')
        self._writer = csv.DictWriter(self._f, fieldnames=fieldnames)
        self._writer.writeheader()

    def write(self, **row):
        self._writer.writerow(row)
        self._f.flush()

    def close(self):
        self._f.close()


from jpype.benchmark import DeepBench

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_ragged_results.txt'
csv_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(out_path) or '.', 'array_ragged_results.csv')

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
]

csv_log = CsvLog(
    csv_path,
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list_ragged(dims, avg_n, seed=0, leaf=int):
    """Sibling lengths vary uniformly in [avg_n-4, avg_n+4] at every
    level (including the leaf level) -- genuinely ragged at every depth,
    not just the outermost. Fixed seed so repeated runs (e.g. before vs.
    after a code change) build the exact same tree.

    leaf converts each leaf value -- float/double targets get genuine
    Python floats here rather than ints Java would otherwise have to
    widen (unlike jpype, jep's push path has no leaf-exactness check to
    accidentally fall off of, but using real floats keeps this a fair
    like-for-like comparison against jpype's/jpy's ragged benchmarks)."""
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


with open(out_path, 'w') as f:
    def run(name, fn, total_elements, dtype, dims):
        n, warmup = calls_for(total_elements)
        best, median = timeit(fn, n=n, warmup=warmup)
        f.write(format_row(name, best, median) + "\n")
        csv_log.write(category='array_ragged', direction='push', source='list',
                      dtype=dtype, dims=dims, size=total_elements, n=n,
                      best_ns=best, median_ns=median)

    for label, SUM_BY_DIMS in TYPES:
        leaf = float if label in ('float', 'double') else int
        f.write(f"=== jep: ragged list->array, multi-dimensional, push (Python -> Java), {label} ===\n")
        for dims in DIMS:
            lst = nested_list_ragged(dims, 10, seed=dims, leaf=leaf)
            size = count_elements(lst, dims)
            sumfn = SUM_BY_DIMS[dims]
            f.write(f"  (dims={dims}, actual element count={size})\n")
            run(f"ragged list->array {label}{'[]' * dims}(~10^{dims}), fresh",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), size, label, dims)

csv_log.close()
