"""Multi-dimensional array conversion, jep side, 2D through 5D, holding
total element count fixed at each depth (matching array_flat.py's sizes).
Swept across four primitive element types (int32, int64, float32,
float64), matching jpype/array_multidim.py's type sweep. Companion:
jpype/array_multidim.py, jpy/array_multidim.py -- same operations and
depths, using the shared jpype.benchmark.DeepBench test class. See
../array_flat.py and ../README.md.

Two categories per direction, not one "arrays" bucket -- see
../array_flat.py for why list vs. buffer input matter as a distinction at
all. jep's own real, source-confirmed limitations (jep_numpy.c/
pyjarray.c) shape both, for every element type:

  - push, "list->array": plain nested Python lists work (jep's general
    per-element recursion, pyfastsequence_as_jobject) -- O(elements)
    work, all inside a single JNI call.
  - push, "buffer->array": jep's numpy fast path only ever targets a flat
    1D primitive array type -- passing a numpy array of any ndim as an
    int[][]-or-deeper (or long[][]/float[][]/double[][]) argument always
    raises "Error matching ndarray.dtype to Java primitive type" (jep's
    own multi-dimensional support is the separate jep.NDArray Java
    class, not a real int[][]), so there's no automatic version of this
    row to measure, for any type. What's measured instead is the best
    manual workaround: jep *does* still have its real numpy fast path
    available at the leaf (int[]/long[]/float[]/double[]) level, so this
    builds the genuine multi-dim Java array by hand, one row at a time --
    each innermost row bulk-converted via its own
    `DeepBench.identity{Type}Array(numpy_row)` call (jep's real fast
    path, confirmed in array_flat.py's push numbers), with the nesting
    above that pure Python-side `jep.jarray()` container construction +
    element assignment.

    At the row length this file uses (10, so the fixed element count is
    spread over more, smaller rows as depth increases), this manual
    "buffer->array" was originally measured, for int, to be *slower*
    than "list->array", by 3-4x, not faster: each row/container step
    here is a separate Python-level call across the JNI boundary, and at
    only 10 elements a row, that per-call overhead (Python call dispatch,
    JNI entry, method resolution) outweighs the bulk-conversion time
    it's saving. jep's single-call nested-list recursion does the
    equivalent per-element work without ever returning to Python
    bytecode in between. Now that the sweep covers long/float/double as
    well, that finding is measured per type below rather than assumed to
    generalize from int -- element size/width could plausibly shift
    where the crossover falls, so treat each type's numbers on their own
    rather than assuming they all match the original int-only result.
    The technique should cross over to a real win once rows are large
    enough for the saved per-element conversion cost to exceed the added
    per-row call overhead -- just not at this size. Kept anyway because
    the finding itself (manual buffer assembly is not automatically a
    win over the naive list path) is the useful result, not a specific
    claim about which type or size it flips at.
  - pull, "array->list"/"array->buffer": every returned array is a
    `pyjarray` with no buffer-protocol support at any depth or type
    (confirmed: no getbufferproc in pyjarray.c; only jep.NDArray gets an
    automatic numpy conversion on return, and that requires the Java
    side to construct a jep.NDArray in the first place, not a real
    int[]/int[][]/etc.), so there's no "manual pieces" trick available
    for pull the way there is for push -- both rows go through the same
    generic per-element sequence path, recursing through nested
    pyjarray-of-pyjarray objects. Kept as two rows anyway for a direct
    comparison against jpype's/jpy's rows of the same name.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.
For the same reason, _common.CsvLog can't be imported either -- see the
inlined CsvLog class below.

Writes a CSV (fieldnames: category, direction, source, dtype, dims,
size, n, best_ns, median_ns) to the path given as argv[2], defaulting to
"array_multidim_results.csv" next to out_path.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_multidim.py <output_path> [<csv_path>]
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

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_multidim_results.txt'
csv_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(out_path) or '.', 'array_multidim_results.csv')

DIMS = [2, 3, 4, 5]

# (label, numpy dtype, identity{Type}Array, {sum2D..sum5D}, {make2D..make5D})
TYPES = [
    ('int', np.dtype('int32'), DeepBench.identityIntArray, {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }, {
        2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
        4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
    }),
    ('long', np.dtype('int64'), DeepBench.identityLongArray, {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }, {
        2: DeepBench.make2DLongArray, 3: DeepBench.make3DLongArray,
        4: DeepBench.make4DLongArray, 5: DeepBench.make5DLongArray,
    }),
    ('float', np.dtype('float32'), DeepBench.identityFloatArray, {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }, {
        2: DeepBench.make2DFloatArray, 3: DeepBench.make3DFloatArray,
        4: DeepBench.make4DFloatArray, 5: DeepBench.make5DFloatArray,
    }),
    ('double', np.dtype('float64'), DeepBench.identityDoubleArray, {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
        4: DeepBench.void4DDoubleArray, 5: DeepBench.void5DDoubleArray,
    }, {
        2: DeepBench.make2DDoubleArray, 3: DeepBench.make3DDoubleArray,
        4: DeepBench.make4DDoubleArray, 5: DeepBench.make5DDoubleArray,
    }),
]

csv_log = CsvLog(
    csv_path,
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list(dims, n, leaf=int):
    """A plain nested Python list of the given depth/side-length -- the
    "list->array" input for a multi-dimensional Java primitive array
    argument (see module docstring). leaf converts each leaf value, so
    float/double targets get genuine Python floats rather than ints Java
    would otherwise have to widen."""
    if dims == 1:
        return [leaf(i) for i in range(n)]
    return [nested_list(dims - 1, n, leaf) for _ in range(n)]


def to_nested_list(ja, dims):
    """Fully materialize a jep multi-dimensional pyjarray into plain
    (recursive) Python lists, for a fair comparison against array->buffer
    -- a shallow list(ja) would only give a list of pyjarray sub-array
    objects, not plain values, at any depth beyond 1."""
    if dims == 1:
        return list(ja)
    return [to_nested_list(row, dims - 1) for row in ja]


# Container element-type classes for manual assembly, keyed by (type
# label, depth) now instead of just depth -- one cache per element type
# since int[][], long[][], float[][], double[][] etc. are all distinct
# Java runtime classes. jep has no string-based array class lookup
# exposed to Python, so each is obtained by building one real, minimal
# instance of that (type, depth) and asking it for its runtime class.
_dim_class = {}


def _dummy(label, depth, identityfn, row_sample):
    if depth == 1:
        return identityfn(row_sample)
    c = jep.jarray(1, _dim_class[(label, depth - 1)])
    c[0] = _dummy(label, depth - 1, identityfn, row_sample)
    return c


for _label, _dtype, _identityfn, _SUM_BY_DIMS, _MAKE_BY_DIMS in TYPES:
    _row_sample = np.zeros(1, dtype=_dtype)
    _dim_class[(_label, 1)] = _identityfn(_row_sample).getClass()
    for _d in range(2, max(DIMS)):
        _dim_class[(_label, _d)] = _dummy(_label, _d, _identityfn, _row_sample).getClass()


def build_manual(np_sub, depth, label, identityfn):
    """Manually assemble a genuine Java int[]/int[][]/... (or
    long/float/double equivalent) array from a numpy array, one row at a
    time (see module docstring). label/identityfn select which type's
    identity{Type}Array/_dim_class entries to use at the leaf and at each
    container level."""
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
    def run(name, fn, total_elements, direction, source, dtype, dims):
        n, warmup = calls_for(total_elements)
        best, median = timeit(fn, n=n, warmup=warmup)
        f.write(format_row(name, best, median) + "\n")
        csv_log.write(category='array_multidim', direction=direction, source=source,
                      dtype=dtype, dims=dims, size=total_elements, n=n,
                      best_ns=best, median_ns=median)

    for label, dtype, identityfn, SUM_BY_DIMS, MAKE_BY_DIMS in TYPES:
        leaf = float if label in ('float', 'double') else int

        f.write(f"=== jep: list->array, multi-dimensional, push (Python -> Java), {label} ===\n")
        for dims in DIMS:
            size = 10 ** dims
            lst = nested_list(dims, 10, leaf)
            sumfn = SUM_BY_DIMS[dims]
            run(f"list->array {label}{'[]' * dims}(10^{dims}), fresh",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
                'push', 'list', label, dims)

        f.write(f"=== jep: buffer->array, multi-dimensional, push (Python -> Java, manual per-row), {label} ===\n")
        for dims in DIMS:
            size = 10 ** dims
            arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
            sumfn = SUM_BY_DIMS[dims]
            run(f"buffer->array {label}{'[]' * dims}(10^{dims}), manual per-row",
                lambda arr=arr, dims=dims, sumfn=sumfn, label=label, identityfn=identityfn:
                    sumfn(build_manual(arr, dims, label, identityfn)), size,
                'push', 'buffer_manual', label, dims)

        f.write(f"=== jep: array->list, multi-dimensional, pull (Java -> Python), {label} ===\n")
        for dims in DIMS:
            size = 10 ** dims
            makefn = MAKE_BY_DIMS[dims]
            run(f"array->list {label}{'[]' * dims}(10^{dims})",
                lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size,
                'pull', 'list', label, dims)

        f.write(f"=== jep: array->buffer, multi-dimensional, pull (Java -> Python), {label} ===\n")
        for dims in DIMS:
            size = 10 ** dims
            makefn = MAKE_BY_DIMS[dims]
            run(f"array->buffer {label}{'[]' * dims}(10^{dims})",
                lambda makefn=makefn: np.asarray(makefn(10)), size,
                'pull', 'buffer', label, dims)

csv_log.close()
