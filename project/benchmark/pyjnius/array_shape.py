"""Shape-at-fixed-depth-and-total sweep, pyjnius side, push only (Python ->
Java) -- isolates row count from row length, which array_multidim.py's
depth sweep can't show since it always uses a uniform, equal-length-per-
dimension shape (10 per dimension). A genuinely lopsided array -- e.g.
100,000 rows of 3 elements each vs. 1,000 rows of 1,000 elements each,
both roughly the same order of total data -- never gets exercised there.

Companion to jpype/array_shape.py, same SHAPES_2D/SHAPES_3D lists and same
`nested_list_shaped` helper. jpype's version sweeps two push categories
per shape -- list->array and buffer->array -- because JPype has a real
buffer->array push path whose per-row cost (or lack of it) is exactly
what that second row is checking. pyjnius has **no buffer->array push at
all, at any size or depth** -- confirmed empirically, not assumed:
passing a numpy array where a Java array argument is expected raises
`JavaException('Expecting a python list/tuple, got array(...)')`
unconditionally (same finding documented in array_flat.py's docstring).
So this file has only ONE push category, not two: list->array via
`nested_list_shaped(shape, leaf)`. There is no buffer->array row to
include here, and no shape-dependence question to ask about a push path
that doesn't exist.

2D shapes, total element count fixed at 100,000 per shape (a clean-divisor
sweep from row-heavy to column-heavy), plus two shapes lifted directly
from the question that motivated jpype's version (unequal totals, kept
as-is since the point was the shapes themselves, not a matched total):
  (10, 10000), (100, 1000), (1000, 100), (10000, 10),
  (3, 100000), (100000, 3), (1000, 1000)

3D shapes, total element count fixed at 100,000:
  (1000, 10, 10) -- outer-heavy (most rows at the outermost level)
  (10, 10, 1000) -- inner-heavy (most elements packed into the innermost
  leaf level, few outer-level recursions)

Pull (Java -> Python) is not covered here: DeepBench's make*Array methods
only build square (equal-length-per-dimension) arrays, and a shape sweep
needs a shape-parameterized factory this harness doesn't have yet -- same
reason jpype's array_shape.py omits pull.

Reports both ns/call and ns/element (best_ns / total elements) side by
side, since the 2D sweep's two motivating shapes have different totals
(300,000 vs. 1,000,000) and aren't otherwise comparable.

Writes project/benchmark/pyjnius/array_shape_results.csv alongside the
printed output.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/array_shape.py \
        [classes_dir] [harness_dir]
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import jnius_config

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'
jnius_config.set_classpath(classes_dir, harness_dir)

from jnius import autoclass

DeepBench = autoclass('jpype.benchmark.DeepBench')

SHAPES_2D = [(10, 10000), (100, 1000), (1000, 100), (10000, 10),
             (3, 100000), (100000, 3), (1000, 1000)]
SHAPES_3D = [(1000, 10, 10), (10, 10, 1000)]

TYPES = [
    ('int', {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
    }),
    ('long', {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
    }),
    ('float', {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
    }),
    ('double', {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_shape_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'shape', 'size',
     'n', 'best_ns', 'median_ns', 'ns_per_element'])


def nested_list_shaped(shape, leaf=int):
    """leaf must produce a genuine (exact-type) Python float for a
    float[]/double[] target -- mirrors jpype's array_shape.py leaf-type
    discipline so the two side-by-side CSVs describe the exact same input
    shapes per type (pyjnius's general per-element conversion accepts a
    plain Python int for a float[]/double[] target too, Java widens it,
    but this keeps the comparison apples-to-apples)."""
    if len(shape) == 1:
        return [leaf(i) for i in range(shape[0])]
    return [nested_list_shaped(shape[1:], leaf) for _ in range(shape[0])]


def total_elements(shape):
    n = 1
    for s in shape:
        n *= s
    return n


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, shape, source, dtype):
    size = total_elements(shape)
    n, warmup = calls_for(size)
    best, median = timeit(fn, n=n, warmup=warmup)
    ns_per_element = best / size
    print(format_row(name, best, median) + f"  ({ns_per_element:6.2f} ns/element)")
    csv_log.write(category='array_shape', direction='push', source=source,
                   dtype=dtype, dims=len(shape), shape='x'.join(str(s) for s in shape),
                   size=size, n=n, best_ns=best, median_ns=median,
                   ns_per_element=ns_per_element)


for label, sumfn_by_dims in TYPES:
    leaf = float if label in ('float', 'double') else int

    print(f"=== pyjnius: list->array, shape sweep, 2D, push (Python -> Java), {label} ===")
    for shape in SHAPES_2D:
        lst = nested_list_shaped(shape, leaf)
        sumfn = sumfn_by_dims[2]
        run(f"list->array {label}[{shape[0]}][{shape[1]}]",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

    print(f"=== pyjnius: list->array, shape sweep, 3D, push (Python -> Java), {label} ===")
    for shape in SHAPES_3D:
        lst = nested_list_shaped(shape, leaf)
        sumfn = sumfn_by_dims[3]
        run(f"list->array {label}[{shape[0]}][{shape[1]}][{shape[2]}]",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

csv_log.close()
