"""Non-contiguous (strided) buffer-protocol source, jep side, push
(Python -> Java) only -- companion to array_flat.py/array_multidim.py,
which both use contiguous numpy sources for their "buffer->array" rows.
A source that can't provide a C-contiguous view -- a numpy column slice,
a transposed array -- is still a fully valid buffer-protocol object, and
this measures whether pushing it takes a bulk buffer-read path or falls
all the way back to a fully general per-element/per-row walk. Swept
across four primitive element types (int32, int64, float32, float64).

Two categories, and jep's two real limitations (see array_flat.py's and
array_multidim.py's module docstrings) shape both differently:

  - 1D: DeepBench.identity{Type}Array(numpy_column) -- a non-unit-stride
    column slice out of a 2D array. jep's real numpy fast path
    (convert_pyndarray_jprimitivearray) genuinely exists at 1D (unlike
    the ND case below), so this ports directly and actually tests the
    interesting question: does that fast path require a C-contiguous
    buffer and silently fall back to the general per-element path for a
    strided one, or does it handle a non-unit stride directly? The
    numbers below answer that empirically, per type -- not assumed here.

  - ND: jep's numpy fast path only ever targets a flat 1D primitive
    array type (confirmed in array_multidim.py's module docstring) --
    passing *any* numpy array, contiguous or not, as an int[][]-or-deeper
    argument always raises "Error matching ndarray.dtype to Java
    primitive type". So unlike jpype (which has a genuine
    JPConversionMultiArrayBuffer path whose contiguity-handling this row
    exists to test), jep has no automatic ND buffer->array path at all
    for this benchmark to probe the contiguity-handling of directly.

    Rather than skip the ND case outright, this reuses array_multidim.py's
    manual per-row assembly workaround (see that file's module docstring
    for the full explanation of why it exists and its int-only finding)
    but feeds it a *transposed* (non-contiguous) numpy source instead of
    a contiguous one. Concretely: `build_manual` still recurses down to
    one row at a time via `np_sub[i]`, but because the top-level array is
    `np.transpose(...)`'d first, every `np_sub[i]` view handed to the
    leaf-level `DeepBench.identity{Type}Array(...)` call is itself
    non-contiguous (a transposed array's rows are never C-contiguous
    past 1D). That makes this row a genuine (if indirect) test of
    whether jep's real leaf-level numpy fast path -- the same one the 1D
    row above tests -- handles a non-contiguous *row* the same way it
    handles a non-contiguous flat array, layered underneath the same
    per-row Python-level call overhead array_multidim.py's manual
    assembly already pays. It does not test whether jep has some
    contiguity-aware bulk ND path, because it doesn't have one to test;
    it tests contiguity-handling at the one leaf-level fast path jep
    does have, reached via the same manual per-row route as
    array_multidim.py's (still-contiguous) buffer->array row, so the two
    are directly comparable against each other.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.
For the same reason, _common.CsvLog can't be imported either -- see the
inlined CsvLog class below.

Writes a CSV (fieldnames: category, direction, source, dtype, dims,
size, n, best_ns, median_ns) to the path given as argv[2], defaulting to
"array_noncontig_results.csv" next to out_path.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_noncontig.py <output_path> [<csv_path>]
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

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_noncontig_results.txt'
csv_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(out_path) or '.', 'array_noncontig_results.csv')

SIZES = [100, 1_000, 10_000, 100_000]
DIMS = [2, 3, 4, 5]

# (label, numpy dtype, identity{Type}Array [manual-assembly leaf, return
# value used], void{Type}Array [actual flat push benchmark target,
# return value discarded], {void2D..void5D})
TYPES = [
    ('int', np.dtype('int32'), DeepBench.identityIntArray, DeepBench.voidIntArray, {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }),
    ('long', np.dtype('int64'), DeepBench.identityLongArray, DeepBench.voidLongArray, {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }),
    ('float', np.dtype('float32'), DeepBench.identityFloatArray, DeepBench.voidFloatArray, {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }),
    ('double', np.dtype('float64'), DeepBench.identityDoubleArray, DeepBench.voidDoubleArray, {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
        4: DeepBench.void4DDoubleArray, 5: DeepBench.void5DDoubleArray,
    }),
]

csv_log = CsvLog(
    csv_path,
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


# Container element-type classes for manual assembly, keyed by (type
# label, depth) -- same machinery as array_multidim.py's _dim_class/
# _dummy/build_manual, copied in here rather than imported since jep
# scripts don't import each other (each is launched standalone via
# jep.Run, see module docstring). A contiguous 1-element sample is enough
# to derive the runtime class at each depth -- only build_manual's actual
# per-row inputs need to be non-contiguous, not this bootstrapping step.
_dim_class = {}


def _dummy(label, depth, identityfn, row_sample):
    if depth == 1:
        return identityfn(row_sample)
    c = jep.jarray(1, _dim_class[(label, depth - 1)])
    c[0] = _dummy(label, depth - 1, identityfn, row_sample)
    return c


for _label, _dtype, _identityfn, _sumfn_flat, _SUM_BY_DIMS in TYPES:
    _row_sample = np.zeros(1, dtype=_dtype)
    _dim_class[(_label, 1)] = _identityfn(_row_sample).getClass()
    for _d in range(2, max(DIMS)):
        _dim_class[(_label, _d)] = _dummy(_label, _d, _identityfn, _row_sample).getClass()


def build_manual(np_sub, depth, label, identityfn):
    """Manually assemble a genuine Java int[]/int[][]/... (or
    long/float/double equivalent) array from a numpy array, one row at a
    time -- identical in structure to array_multidim.py's build_manual,
    but here np_sub is expected to be a transposed (non-contiguous) view,
    so every leaf-level identityfn(np_sub[i]) call receives a
    non-contiguous 1D row rather than a contiguous one (see module
    docstring)."""
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
    def run(name, fn, total_elements, dtype, dims, source):
        n, warmup = calls_for(total_elements)
        best, median = timeit(fn, n=n, warmup=warmup)
        f.write(format_row(name, best, median) + "\n")
        csv_log.write(category='array_noncontig', direction='push', source=source,
                      dtype=dtype, dims=dims, size=total_elements, n=n,
                      best_ns=best, median_ns=median)

    for label, dtype, identityfn, sumfn_flat, SUM_BY_DIMS in TYPES:
        f.write(f"=== jep: buffer->array, flat (1D), non-contiguous column, push (Python -> Java), {label} ===\n")
        for size in SIZES:
            # A 2-column array's column 0 has stride == 2*itemsize --
            # never C-contiguous regardless of size.
            base = np.arange(size * 2, dtype=dtype).reshape(size, 2)
            col = base[:, 0]
            assert not col.flags['C_CONTIGUOUS']
            run(f"buffer->array {label}[{size}], column slice",
                lambda col=col, sumfn_flat=sumfn_flat: sumfn_flat(col), size,
                label, 1, 'buffer_noncontig')

        f.write(f"=== jep: buffer->array, multi-dimensional, transposed, push (Python -> Java, manual per-row), {label} ===\n")
        for dims in DIMS:
            size = 10 ** dims
            arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
            arr = np.transpose(arr, tuple(reversed(range(dims))))
            assert not arr.flags['C_CONTIGUOUS']
            sumfn = SUM_BY_DIMS[dims]
            run(f"buffer->array {label}{'[]' * dims}(10^{dims}), transposed, manual per-row",
                lambda arr=arr, dims=dims, sumfn=sumfn, label=label, identityfn=identityfn:
                    sumfn(build_manual(arr, dims, label, identityfn)), size,
                label, dims, 'buffer_noncontig_manual')

csv_log.close()
