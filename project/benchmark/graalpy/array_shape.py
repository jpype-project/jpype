"""Shape-at-fixed-depth-and-total sweep, GraalPy side, push only (Python ->
Java) -- isolates row count from row length, matching jpype/array_shape.py.
See ../README.md.

2D shapes, total element count fixed at 100,000 per shape:
  (10, 10000), (100, 1000), (1000, 100), (10000, 10),
  (3, 100000), (100000, 3), (1000, 1000)

3D shapes, total element count fixed at 100,000:
  (1000, 10, 10), (10, 10, 1000)

Two push categories per shape, matching jpype's list->array/buffer->array
pair -- GraalPy's "buffer->array" is ../_arrayutil.py's build_manual()
emulation (see ../array_flat.py's docstring for why it's measured rather
than skipped as a gap), so this sweep also answers a build_manual()
specific question array_multidim.py's fixed-uniform-shape sweep can't:
whether build_manual()'s cost is driven by row *count* (many small
allocate-and-fill steps) or by total element count, at a fixed total.

Reports both ns/call and ns/element (best_ns / total elements) side by
side, same as jpype's.

Writes project/benchmark/graalpy/array_shape_results.csv alongside the
printed output.

The row-heavy 2D shapes -- (100000, 3) and (3, 100000), 100,000 tiny rows
either way round -- can raise a Python-level MemoryError partway through
under a capped GraalPy heap (confirmed: reliably fails at -Xmx3g on this
machine, immediately after the (3, 100000) list->array row, with dozens
of TruffleCompilerThread OutOfMemoryErrors preceding it in the log). This
is a real, measured cost of GraalPy's per-object overhead building
100,000+ small nested list/array objects, not a benchmark-harness bug --
`run()` below catches it per-row (best_ns/median_ns left blank in the
CSV) so one exhausted row doesn't take down the rest of the sweep.
Always cap the JVM heap explicitly when running GraalPy benchmarks on
this machine (see ../README.md) -- an uncapped run doesn't fail cleanly
here, it can consume all 7.7GB of host RAM first (see this repo's
CLAUDE.md and the OOM incident during this harness's own setup).

Usage: run via the Bench launcher with DeepBench on the classpath and a
capped heap (see ../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog
from _arrayutil import build_manual, calls_for_manual

import numpy as np
import java

DeepBench = java.type('jpype.benchmark.DeepBench')

SHAPES_2D = [(10, 10000), (100, 1000), (1000, 100), (10000, 10),
             (3, 100000), (100000, 3), (1000, 1000)]
SHAPES_3D = [(1000, 10, 10), (10, 10, 1000)]

TYPES = [
    ('int', np.dtype('int32'), {
        2: DeepBench.sum2DIntArray, 3: DeepBench.sum3DIntArray,
    }),
    ('long', np.dtype('int64'), {
        2: DeepBench.sum2DLongArray, 3: DeepBench.sum3DLongArray,
    }),
    ('float', np.dtype('float32'), {
        2: DeepBench.sum2DFloatArray, 3: DeepBench.sum3DFloatArray,
    }),
    ('double', np.dtype('float64'), {
        2: DeepBench.sum2DDoubleArray, 3: DeepBench.sum3DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_shape_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'shape', 'size',
     'n', 'best_ns', 'median_ns', 'ns_per_element'])


def nested_list_shaped(shape, leaf=int):
    if len(shape) == 1:
        return [leaf(i) for i in range(shape[0])]
    return [nested_list_shaped(shape[1:], leaf) for _ in range(shape[0])]


def total_elements(shape):
    n = 1
    for s in shape:
        n *= s
    return n


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, shape, source, dtype, manual=False):
    size = total_elements(shape)
    n, warmup = calls_for_manual(size) if manual else calls_for(size)
    best, median = timeit(fn, n=n, warmup=warmup)
    ns_per_element = best / size
    print(format_row(name, best, median) + f"  ({ns_per_element:6.2f} ns/element)")
    csv_log.write(category='array_shape', direction='push', source=source,
                   dtype=dtype, dims=len(shape), shape='x'.join(str(s) for s in shape),
                   size=size, n=n, best_ns=best, median_ns=median,
                   ns_per_element=ns_per_element)


def skip(name, shape, source, dtype):
    # Row-heavy shapes (many small rows, e.g. 100000x3) can exhaust a
    # capped GraalPy heap outright, either while just building the
    # nested-list/array input or during the push itself -- see module
    # docstring. A genuine, measured limitation, not a bug to paper over:
    # recorded as a skip (best_ns/median_ns left blank) so the rest of
    # the sweep still runs, rather than the whole file dying on one row.
    print(f"{name:32s} SKIPPED (MemoryError)")
    csv_log.write(category='array_shape', direction='push', source=source,
                   dtype=dtype, dims=len(shape), shape='x'.join(str(s) for s in shape),
                   size=total_elements(shape), n='', best_ns='', median_ns='', ns_per_element='')


for label, dtype, sumfn_by_dims in TYPES:
    leaf = float if label in ('float', 'double') else int

    print(f"=== GraalPy: list->array, shape sweep, 2D, push (Python -> Java), {label} ===")
    for shape in SHAPES_2D:
        name = f"list->array {label}[{shape[0]}][{shape[1]}]"
        try:
            lst = nested_list_shaped(shape, leaf)
            sumfn = sumfn_by_dims[2]
            run(name, lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)
        except MemoryError:
            skip(name, shape, 'list', label)

    print(f"=== GraalPy: buffer->array (manual), shape sweep, 2D, push (Python -> Java), {label} ===")
    for shape in SHAPES_2D:
        name = f"buffer->array {label}[{shape[0]}][{shape[1]}], manual"
        try:
            arr = np.arange(total_elements(shape), dtype=dtype).reshape(shape)
            sumfn = sumfn_by_dims[2]
            run(name, lambda arr=arr, sumfn=sumfn, label=label:
                sumfn(build_manual(arr, 2, label)), shape, 'buffer_manual', label, manual=True)
        except MemoryError:
            skip(name, shape, 'buffer_manual', label)

    print(f"=== GraalPy: list->array, shape sweep, 3D, push (Python -> Java), {label} ===")
    for shape in SHAPES_3D:
        lst = nested_list_shaped(shape, leaf)
        sumfn = sumfn_by_dims[3]
        run(f"list->array {label}[{shape[0]}][{shape[1]}][{shape[2]}]",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

    print(f"=== GraalPy: buffer->array (manual), shape sweep, 3D, push (Python -> Java), {label} ===")
    for shape in SHAPES_3D:
        arr = np.arange(total_elements(shape), dtype=dtype).reshape(shape)
        sumfn = sumfn_by_dims[3]
        run(f"buffer->array {label}[{shape[0]}][{shape[1]}][{shape[2]}], manual",
            lambda arr=arr, sumfn=sumfn, label=label:
                sumfn(build_manual(arr, 3, label)), shape, 'buffer_manual', label, manual=True)

csv_log.close()
