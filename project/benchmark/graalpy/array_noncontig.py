"""Non-contiguous (strided) buffer-protocol source, GraalPy side, push
(Python -> Java) only -- companion to array_flat.py/array_multidim.py.

Unlike pyjnius (which has no buffer->array push at all, so this file is a
no-op stub there -- see ../pyjnius/array_noncontig.py), GraalPy's missing
buffer->array push is emulated via ../_arrayutil.py's build_manual() (see
../array_flat.py's docstring), and build_manual() indexes the numpy
source with plain `np_sub[i]`, which numpy itself resolves through
whatever strides the source has -- so there IS a real question to ask
here: does a non-contiguous source cost build_manual() any more than a
contiguous one of the same shape? (Expected: no, or close to it --
build_manual() is already a per-element Python-level walk with no bulk
buffer read to lose in the first place, so there's no fast path for
non-contiguity to knock it off of.)

Two categories, matching jpype/array_noncontig.py:
  - 1D: DeepBench.sum{Type}Array(build_manual(numpy_column, 1, label)) --
    a non-unit-stride column slice out of a 2D array.
  - ND: DeepBench.sum{2,3,4,5}D{Type}Array(build_manual(transposed_numpy_array,
    dims, label)) -- np.transpose with a reversed axis order, guaranteed
    non-contiguous past 1D.

Writes project/benchmark/graalpy/array_noncontig_results.csv alongside
the printed output.

Usage: run via the Bench launcher with DeepBench on the classpath (see
../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import format_row, CsvLog, timeit
from _arrayutil import build_manual, calls_for_manual

import numpy as np
import java

DeepBench = java.type('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]
DIMS = [2, 3, 4, 5]

TYPES = [
    ('int', np.dtype('int32'), DeepBench.sumIntArray, {
        2: DeepBench.sum2DIntArray, 3: DeepBench.sum3DIntArray,
        4: DeepBench.sum4DIntArray, 5: DeepBench.sum5DIntArray,
    }),
    ('long', np.dtype('int64'), DeepBench.sumLongArray, {
        2: DeepBench.sum2DLongArray, 3: DeepBench.sum3DLongArray,
        4: DeepBench.sum4DLongArray, 5: DeepBench.sum5DLongArray,
    }),
    ('float', np.dtype('float32'), DeepBench.sumFloatArray, {
        2: DeepBench.sum2DFloatArray, 3: DeepBench.sum3DFloatArray,
        4: DeepBench.sum4DFloatArray, 5: DeepBench.sum5DFloatArray,
    }),
    ('double', np.dtype('float64'), DeepBench.sumDoubleArray, {
        2: DeepBench.sum2DDoubleArray, 3: DeepBench.sum3DDoubleArray,
        4: DeepBench.sum4DDoubleArray, 5: DeepBench.sum5DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_noncontig_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def run(name, fn, total_elements, dtype, dims):
    n, warmup = calls_for_manual(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_noncontig', direction='push', source='buffer_manual_noncontig',
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


for label, dtype, sumfn_flat, SUM_BY_DIMS in TYPES:
    print(f"=== GraalPy: buffer->array (manual), flat (1D), non-contiguous column, push (Python -> Java), {label} ===")
    for size in SIZES:
        # A 2-column array's column 0 has stride == 2*itemsize -- never
        # C-contiguous regardless of size.
        base = np.arange(size * 2, dtype=dtype).reshape(size, 2)
        col = base[:, 0]
        assert not col.flags['C_CONTIGUOUS']
        run(f"buffer->array {label}[{size}], manual, column slice",
            lambda col=col, sumfn_flat=sumfn_flat, label=label:
                sumfn_flat(build_manual(col, 1, label)), size, label, 1)

    print(f"=== GraalPy: buffer->array (manual), multi-dimensional, transposed, push (Python -> Java), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
        arr = np.transpose(arr, tuple(reversed(range(dims))))
        assert not arr.flags['C_CONTIGUOUS']
        sumfn = SUM_BY_DIMS[dims]
        run(f"buffer->array {label}{'[]' * dims}(10^{dims}), manual, transposed",
            lambda arr=arr, sumfn=sumfn, label=label, dims=dims:
                sumfn(build_manual(arr, dims, label)), size, label, dims)

csv_log.close()
