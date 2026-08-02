"""Multi-dimensional array conversion, jep side, 2D through 5D, holding
total element count fixed at each depth (matching array_flat.py's sizes).
Companion: jpype/array_multidim.py, jpy/array_multidim.py -- same
operations and depths, using the shared jpype.benchmark.DeepBench test
class. See ../array_flat.py and ../README.md.

Two categories per direction, not one "arrays" bucket -- see
../array_flat.py for why list vs. buffer input matter as a distinction at
all. jep's own real, source-confirmed limitations (jep_numpy.c/
pyjarray.c) shape both:

  - push, "list->array": plain nested Python lists work (jep's general
    per-element recursion, pyfastsequence_as_jobject) -- O(elements)
    work, all inside a single JNI call.
  - push, "buffer->array": jep's numpy fast path only ever targets a flat
    1D primitive array type -- passing a numpy array of any ndim as an
    int[][]-or-deeper argument always raises "Error matching
    ndarray.dtype to Java primitive type" (jep's own multi-dimensional
    support is the separate jep.NDArray Java class, not a real int[][]),
    so there's no automatic version of this row to measure. What's
    measured instead is the best manual workaround: jep *does* still
    have its real numpy fast path available at the leaf (int[]) level,
    so this builds the genuine multi-dim Java array by hand, one row at
    a time -- each innermost row bulk-converted via its own
    `DeepBench.identityIntArray(numpy_row)` call (jep's real fast path,
    confirmed in array_flat.py's push numbers), with the nesting above
    that pure Python-side `jep.jarray()` container construction +
    element assignment.

    At the row length this file uses (10, so the fixed element count is
    spread over more, smaller rows as depth increases), this manual
    "buffer->array" is measured to be *slower* than "list->array", by
    3-4x, not faster (measured, not assumed): each row/container step
    here is a separate Python-level call across the JNI boundary, and at
    only 10 elements a row, that per-call overhead (Python call dispatch,
    JNI entry, method resolution) outweighs the bulk-conversion time
    it's saving. jep's single-call nested-list recursion does the
    equivalent per-element work without ever returning to Python
    bytecode in between. The technique should cross over to a real win
    once rows are large enough for the saved per-element conversion cost
    to exceed the added per-row call overhead -- just not at this size.
    Kept anyway because the finding itself (manual buffer assembly is
    not automatically a win over the naive list path) is the useful
    result, not a specific claim about which one is faster in general.
  - pull, "array->list"/"array->buffer": every returned array is a
    `pyjarray` with no buffer-protocol support at any depth (confirmed:
    no getbufferproc in pyjarray.c; only jep.NDArray gets an automatic
    numpy conversion on return, and that requires the Java side to
    construct a jep.NDArray in the first place, not a real
    int[]/int[][]/etc.), so there's no "manual pieces" trick available
    for pull the way there is for push -- both rows go through the same
    generic per-element sequence path, recursing through nested
    pyjarray-of-pyjarray objects. Kept as two rows anyway for a direct
    comparison against jpype's/jpy's rows of the same name.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_multidim.py <output_path>
"""
import sys
import time


def timeit(fn, n=200_000, warmup=1000, trials=5):
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


from jpype.benchmark import DeepBench
import numpy as np
import jep

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_multidim_results.txt'

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
    """A plain nested Python list of the given depth/side-length -- the
    "list->array" input for a multi-dimensional Java primitive array
    argument (see module docstring)."""
    if dims == 1:
        return list(range(n))
    return [nested_list(dims - 1, n) for _ in range(n)]


def to_nested_list(ja, dims):
    """Fully materialize a jep multi-dimensional pyjarray into plain
    (recursive) Python lists, for a fair comparison against array->buffer
    -- a shallow list(ja) would only give a list of pyjarray sub-array
    objects, not plain ints, at any depth beyond 1."""
    if dims == 1:
        return list(ja)
    return [to_nested_list(row, dims - 1) for row in ja]


# Container element-type classes for manual assembly, keyed by depth (1 ->
# int[]'s class, 2 -> int[][]'s class, ...). jep has no string-based array
# class lookup exposed to Python, so each is obtained by building one real,
# minimal instance of that depth and asking it for its runtime class.
_row_sample = np.zeros(1, dtype=np.int32)
_dim_class = {1: DeepBench.identityIntArray(_row_sample).getClass()}


def _dummy(depth):
    if depth == 1:
        return DeepBench.identityIntArray(_row_sample)
    c = jep.jarray(1, _dim_class[depth - 1])
    c[0] = _dummy(depth - 1)
    return c


for _d in range(2, max(DIMS)):
    _dim_class[_d] = _dummy(_d).getClass()


def build_manual(np_sub, depth):
    """Manually assemble a genuine Java int[]/int[][]/... array from a
    numpy array, one row at a time (see module docstring)."""
    if depth == 1:
        return DeepBench.identityIntArray(np_sub)
    n = np_sub.shape[0]
    container = jep.jarray(n, _dim_class[depth - 1])
    for i in range(n):
        container[i] = build_manual(np_sub[i], depth - 1)
    return container


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


with open(out_path, 'w') as f:
    def run(name, fn, total_elements):
        n, warmup = calls_for(total_elements)
        best, median = timeit(fn, n=n, warmup=warmup)
        f.write(format_row(name, best, median) + "\n")

    f.write("=== jep: list->array, multi-dimensional, push (Python -> Java) ===\n")
    for dims in DIMS:
        size = 10 ** dims
        lst = nested_list(dims, 10)
        sumfn = SUM_BY_DIMS[dims]
        run(f"list->array int{'[]' * dims}(10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size)

    f.write("=== jep: buffer->array, multi-dimensional, push (Python -> Java, manual per-row) ===\n")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=np.int32).reshape((10,) * dims)
        sumfn = SUM_BY_DIMS[dims]
        run(f"buffer->array int{'[]' * dims}(10^{dims}), manual per-row",
            lambda arr=arr, dims=dims, sumfn=sumfn:
                sumfn(build_manual(arr, dims)), size)

    f.write("=== jep: array->list, multi-dimensional, pull (Java -> Python) ===\n")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->list int{'[]' * dims}(10^{dims})",
            lambda makefn=makefn, dims=dims: to_nested_list(makefn(10), dims), size)

    f.write("=== jep: array->buffer, multi-dimensional, pull (Java -> Python) ===\n")
    for dims in DIMS:
        size = 10 ** dims
        makefn = MAKE_BY_DIMS[dims]
        run(f"array->buffer int{'[]' * dims}(10^{dims})",
            lambda makefn=makefn: np.asarray(makefn(10)), size)
