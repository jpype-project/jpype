"""Shape-at-fixed-depth-and-total sweep, jep side, push only (Python ->
Java) -- isolates row count from row length, which array_multidim.py's
depth sweep can't show since it always uses a uniform, equal-length-per-
dimension shape (10 per dimension). A genuinely lopsided array -- e.g.
100,000 rows of 3 elements each vs. 1,000 rows of 1,000 elements each,
both roughly the same order of total data -- never gets exercised there.
Swept across four primitive element types (int32, int64, float32,
float64), matching jpype/array_shape.py's type sweep.

2D shapes, total element count fixed at 100,000 per shape (a clean-divisor
sweep from row-heavy to column-heavy), plus two shapes lifted directly
from the question that motivated jpype/array_shape.py (unequal totals,
kept as-is since the point was the shapes themselves, not a matched
total) -- identical shape list to jpype's, so the two are directly
comparable row-for-row:
  (10, 10000), (100, 1000), (1000, 100), (10000, 10),
  (3, 100000), (100000, 3), (1000, 1000)

3D shapes, total element count fixed at 100,000:
  (1000, 10, 10) -- outer-heavy (most rows at the outermost level)
  (10, 10, 1000) -- inner-heavy (most elements packed into the innermost
  leaf level, few outer-level recursions)

Two categories per depth, and jep's real limitations shape both
differently (see array_flat.py's and array_multidim.py's module
docstrings for the source-confirmed details):

  - "list->array": a plain (nested) Python list, shaped to the exact
    (possibly lopsided) target shape -- jep's general per-element
    recursion (`pyfastsequence_as_jobject`) ports directly here, same as
    array_multidim.py's list->array row, just with a non-uniform shape
    instead of a uniform one.
  - "buffer->array": jep's numpy fast path only ever targets a flat 1D
    primitive array type, so -- exactly as in array_multidim.py -- there
    is no automatic buffer->array push for any 2D/3D shape here either.
    This reuses array_multidim.py's manual per-row assembly workaround
    (build a genuine Java int[][]/... by hand, one row at a time, each
    row's leaf bulk-converted via `DeepBench.identity{Type}Array`),
    adapted to accept an arbitrary (not just square/uniform) shape --
    `build_manual` below only ever reads `np_sub.shape[0]` at each level,
    so a lopsided shape like (100000, 3) or (3, 100000) needs no special
    handling beyond what array_multidim.py's version already does; it
    falls out of the same recursion. Kept as a real row here (not
    skipped) because the row-count-vs-row-length question this file
    exists to answer is exactly the kind of thing manual per-row
    assembly's per-row call overhead should be sensitive to -- e.g.
    (100000, 3) pays that per-row Python/JNI round-trip overhead 100,000
    times for only 3 elements of payoff each time, while (3, 100000)
    pays it only 3 times for 100,000 elements each; whether that shows up
    as a large gap between those two rows' timings is exactly what this
    sweep is for.

Pull (Java -> Python) is not covered here: DeepBench's make*Array methods
only build square (equal-length-per-dimension) arrays, and a shape sweep
needs a shape-parameterized factory this harness doesn't have yet (same
limitation noted in jpype/array_shape.py).

Reports both ns/call and ns/element (best_ns / total elements) side by
side, matching jpype/array_shape.py, since the 2D sweep's two motivating
shapes have different totals (300,000 vs. 1,000,000) and aren't otherwise
comparable.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.
For the same reason, _common.CsvLog can't be imported either -- see the
inlined CsvLog class below.

Writes a CSV (fieldnames: category, direction, source, dtype, dims,
shape, size, n, best_ns, median_ns, ns_per_element) to the path given as
argv[2], defaulting to "array_shape_results.csv" next to out_path.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_shape.py <output_path> [<csv_path>]
"""
import sys
import os
import csv
import time


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
import numpy as np
import jep

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_shape_results.txt'
csv_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(out_path) or '.', 'array_shape_results.csv')

SHAPES_2D = [(10, 10000), (100, 1000), (1000, 100), (10000, 10),
             (3, 100000), (100000, 3), (1000, 1000)]
SHAPES_3D = [(1000, 10, 10), (10, 10, 1000)]

DIMS = [2, 3]

# (label, numpy dtype, identity{Type}Array, {sum2D, sum3D})
TYPES = [
    ('int', np.dtype('int32'), DeepBench.identityIntArray, {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
    }),
    ('long', np.dtype('int64'), DeepBench.identityLongArray, {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
    }),
    ('float', np.dtype('float32'), DeepBench.identityFloatArray, {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
    }),
    ('double', np.dtype('float64'), DeepBench.identityDoubleArray, {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
    }),
]

csv_log = CsvLog(
    csv_path,
    ['category', 'direction', 'source', 'dtype', 'dims', 'shape', 'size',
     'n', 'best_ns', 'median_ns', 'ns_per_element'])


def nested_list_shaped(shape, leaf=int):
    """leaf converts each leaf value -- float/double targets get genuine
    Python floats here rather than ints Java would otherwise have to
    widen (see array_multidim.py's nested_list for the same convention)."""
    if len(shape) == 1:
        return [leaf(i) for i in range(shape[0])]
    return [nested_list_shaped(shape[1:], leaf) for _ in range(shape[0])]


def total_elements(shape):
    n = 1
    for s in shape:
        n *= s
    return n


# Container element-type classes for manual assembly, keyed by (type
# label, depth) -- same machinery as array_multidim.py's _dim_class/
# _dummy/build_manual, copied in here rather than imported since jep
# scripts don't import each other (each is launched standalone via
# jep.Run). Only depth 1 (the identityfn's own class) is needed directly
# by this file's build_manual, since the shapes here only go to depth 3,
# but the same recursive bootstrap as array_multidim.py's is used for
# consistency and in case DIMS grows later.
_dim_class = {}


def _dummy(label, depth, identityfn, row_sample):
    if depth == 1:
        return identityfn(row_sample)
    c = jep.jarray(1, _dim_class[(label, depth - 1)])
    c[0] = _dummy(label, depth - 1, identityfn, row_sample)
    return c


for _label, _dtype, _identityfn, _SUM_BY_DIMS in TYPES:
    _row_sample = np.zeros(1, dtype=_dtype)
    _dim_class[(_label, 1)] = _identityfn(_row_sample).getClass()
    for _d in range(2, max(DIMS)):
        _dim_class[(_label, _d)] = _dummy(_label, _d, _identityfn, _row_sample).getClass()


def build_manual(np_sub, depth, label, identityfn):
    """Manually assemble a genuine Java int[][]/long[][]/float[][]/
    double[][] (or 3D equivalent) array from a numpy array, one row at a
    time -- structurally identical to array_multidim.py's build_manual
    (see that file's module docstring for the full rationale), but here
    np_sub's shape is whatever arbitrary (possibly lopsided) shape this
    file is sweeping rather than always a uniform 10-per-dimension one;
    build_manual only ever reads np_sub.shape[0] at each level so no
    special-casing is needed for that."""
    if depth == 1:
        return identityfn(np_sub)
    n = np_sub.shape[0]
    container = jep.jarray(n, _dim_class[(label, depth - 1)])
    for i in range(n):
        container[i] = build_manual(np_sub[i], depth - 1, label, identityfn)
    return container


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


with open(out_path, 'w') as f:
    def run(name, fn, shape, source, dtype):
        size = total_elements(shape)
        n, warmup = calls_for(size)
        best, median = timeit(fn, n=n, warmup=warmup)
        ns_per_element = best / size
        f.write(format_row(name, best, median) + f"  ({ns_per_element:6.2f} ns/element)\n")
        csv_log.write(category='array_shape', direction='push', source=source,
                      dtype=dtype, dims=len(shape), shape='x'.join(str(s) for s in shape),
                      size=size, n=n, best_ns=best, median_ns=median,
                      ns_per_element=ns_per_element)

    for label, dtype, identityfn, sumfn_by_dims in TYPES:
        leaf = float if label in ('float', 'double') else int

        f.write(f"=== jep: list->array, shape sweep, 2D, push (Python -> Java), {label} ===\n")
        for shape in SHAPES_2D:
            lst = nested_list_shaped(shape, leaf)
            sumfn = sumfn_by_dims[2]
            run(f"list->array {label}[{shape[0]}][{shape[1]}]",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

        f.write(f"=== jep: buffer->array, shape sweep, 2D, push (Python -> Java, manual per-row), {label} ===\n")
        for shape in SHAPES_2D:
            arr = np.arange(total_elements(shape), dtype=dtype).reshape(shape)
            sumfn = sumfn_by_dims[2]
            run(f"buffer->array {label}[{shape[0]}][{shape[1]}], manual per-row",
                lambda arr=arr, sumfn=sumfn, label=label, identityfn=identityfn:
                    sumfn(build_manual(arr, 2, label, identityfn)), shape, 'buffer_manual', label)

        f.write(f"=== jep: list->array, shape sweep, 3D, push (Python -> Java), {label} ===\n")
        for shape in SHAPES_3D:
            lst = nested_list_shaped(shape, leaf)
            sumfn = sumfn_by_dims[3]
            run(f"list->array {label}[{shape[0]}][{shape[1]}][{shape[2]}]",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

        f.write(f"=== jep: buffer->array, shape sweep, 3D, push (Python -> Java, manual per-row), {label} ===\n")
        for shape in SHAPES_3D:
            arr = np.arange(total_elements(shape), dtype=dtype).reshape(shape)
            sumfn = sumfn_by_dims[3]
            run(f"buffer->array {label}[{shape[0]}][{shape[1]}][{shape[2]}], manual per-row",
                lambda arr=arr, sumfn=sumfn, label=label, identityfn=identityfn:
                    sumfn(build_manual(arr, 3, label, identityfn)), shape, 'buffer_manual', label)

csv_log.close()
