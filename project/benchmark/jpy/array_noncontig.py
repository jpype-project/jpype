"""Non-contiguous (strided) buffer-protocol source, jpy side, push
(Python -> Java) only -- companion to array_flat.py/array_multidim.py,
which both use contiguous numpy sources for their "buffer->array" rows.
A source that can't provide a C-contiguous view -- a numpy column
slice, a transposed array -- is still a fully valid buffer-protocol
object, and this measures whether pushing it takes a bulk buffer-read
path or falls all the way back to a fully general per-element/per-row
walk (or errors outright). Swept across four primitive element types
(int32, int64, float32, float64).

Two categories:
  - 1D: DeepBench.void{Type}Array(numpy_column) -- a non-unit-stride column
    slice out of a 2D array.
  - ND: DeepBench.void{2,3,4,5}D{Type}Array(transposed_numpy_array) --
    np.transpose with a reversed axis order, guaranteed non-contiguous
    past 1D.

**Confirmed by running this file**: the 1D column-slice row raises
`RuntimeError: no matching Java method overloads found` in jpy, for
every type and every size -- not a `BufferError` from numpy's
`bf_getbuffer` as the source read predicted, but the same end result:
jpy's buffer-argument path (`JType_ConvertPyArgToJObjectArg`,
jpy_jtype.c) requests `flags = PyBUF_SIMPLE` (never `PyBUF_ND`/
`PyBUF_STRIDES`) when the *direct* parameter component type is
primitive (i.e. only for a flat 1D target); when that buffer request
fails, jpy's matcher apparently treats the argument as simply
unmatched against any overload, rather than surfacing the underlying
buffer error. The `run()` wrapper below catches this and records a
`None`/`error` row instead of aborting the whole sweep, so the ND
rows (which don't take this branch at all -- confirmed unaffected,
see below) still get measured.

The ND rows are unaffected, as predicted: a multi-dimensional numpy
array never takes jpy's buffer branch (paramComponentType is `int[]`,
not primitive, for an `int[][]` target -- see array_multidim.py's
docstring), so it falls to the same generic per-element/per-row
recursion regardless of contiguity, reading through `__getitem__`,
which is stride-agnostic. This is a real, jpy-specific coverage gap:
jpype and jep (see their array_noncontig.py) both take a non-contiguous
1D buffer source through a working fast/fallback path; jpy's flat
buffer-argument matching cannot accept one at all.

Writes project/benchmark/jpy/array_noncontig_results.csv alongside the
printed output.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/array_noncontig.py \
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
DIMS = [2, 3, 4, 5]

TYPES = [
    ('int', np.dtype('int32'), DeepBench.voidIntArray, {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }),
    ('long', np.dtype('int64'), DeepBench.voidLongArray, {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }),
    ('float', np.dtype('float32'), DeepBench.voidFloatArray, {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }),
    ('double', np.dtype('float64'), DeepBench.voidDoubleArray, {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
        4: DeepBench.void4DDoubleArray, 5: DeepBench.void5DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_noncontig_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, dtype, dims):
    n, warmup = calls_for(total_elements)
    try:
        best, median = timeit(fn, n=n, warmup=warmup)
    except RuntimeError as e:
        # jpy's flat buffer-argument matching cannot accept a
        # non-contiguous 1D source at all (see module docstring) --
        # record the failure instead of aborting the whole sweep, so
        # the (unaffected) ND rows still get measured.
        print(f"{name:32s} FAILED: {e}")
        csv_log.write(category='array_noncontig', direction='push', source='buffer_noncontig',
                       dtype=dtype, dims=dims, size=total_elements, n=n,
                       best_ns='', median_ns='')
        return
    print(format_row(name, best, median))
    csv_log.write(category='array_noncontig', direction='push', source='buffer_noncontig',
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


for label, dtype, sumfn_flat, SUM_BY_DIMS in TYPES:
    print(f"=== jpy: buffer->array, flat (1D), non-contiguous column, push (Python -> Java), {label} ===")
    for size in SIZES:
        # A 2-column array's column 0 has stride == 2*itemsize -- never
        # C-contiguous regardless of size.
        base = np.arange(size * 2, dtype=dtype).reshape(size, 2)
        col = base[:, 0]
        assert not col.flags['C_CONTIGUOUS']
        run(f"buffer->array {label}[{size}], column slice",
            lambda col=col, sumfn_flat=sumfn_flat: sumfn_flat(col), size, label, 1)

    print(f"=== jpy: buffer->array, multi-dimensional, transposed, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
        arr = np.transpose(arr, tuple(reversed(range(dims))))
        assert not arr.flags['C_CONTIGUOUS']
        sumfn = SUM_BY_DIMS[dims]
        run(f"buffer->array {label}{'[]' * dims}(10^{dims}), transposed",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), size, label, dims)

csv_log.close()
