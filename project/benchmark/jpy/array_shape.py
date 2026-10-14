"""Shape-at-fixed-depth-and-total sweep, jpy side, push only (Python ->
Java) -- isolates row count from row length, which array_multidim.py's
depth sweep can't show since it always uses a uniform, equal-length-per-
dimension shape (10 per dimension). A genuinely lopsided array -- e.g.
100,000 rows of 3 elements each vs. 1,000 rows of 1,000 elements each,
both roughly the same order of total data -- never gets exercised there.

Why this might matter: jpy's per-element recursion
(JType_CreateJavaArray, jpy_jtype.c) does one unit of work per *row* (a
Python-level sub-sequence access, plus a JNI sub-array allocation) in
addition to the per-element work each row's elements need, same as
jpype's JPConversionSequence does. A shape with many short rows pays
that per-row cost far more often, for the same total element count, than
a shape with few long rows. This sweep measures whether that shows up,
rather than assuming it does.

2D shapes, total element count fixed at 100,000 per shape (a clean-divisor
sweep from row-heavy to column-heavy), plus two shapes lifted directly
from the question that motivated this file (unequal totals, kept as-is
since the point was the shapes themselves, not a matched total):
  (10, 10000), (100, 1000), (1000, 100), (10000, 10),
  (3, 100000), (100000, 3), (1000, 1000)

3D shapes, total element count fixed at 100,000:
  (1000, 10, 10) -- outer-heavy (most rows at the outermost level)
  (10, 10, 1000) -- inner-heavy (most elements packed into the innermost
  leaf level, few outer-level recursions)

Pull (Java -> Python) is not covered here: DeepBench's make*Array methods
only build square (equal-length-per-dimension) arrays, and a shape sweep
needs a shape-parameterized factory this harness doesn't have yet. push
is where the question originated, and where the per-row recursion
actually lives.

Reports both ns/call and ns/element (best_ns / total elements) side by
side, since the 2D sweep's two motivating shapes have different totals
(300,000 vs. 1,000,000) and aren't otherwise comparable.

Writes project/benchmark/jpy/array_shape_results.csv alongside the
printed output.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/array_shape.py \
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

SHAPES_2D = [(10, 10000), (100, 1000), (1000, 100), (10000, 10),
             (3, 100000), (100000, 3), (1000, 1000)]
SHAPES_3D = [(1000, 10, 10), (10, 10, 1000)]

TYPES = [
    ('int', np.dtype('int32'), {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
    }),
    ('long', np.dtype('int64'), {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
    }),
    ('float', np.dtype('float32'), {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
    }),
    ('double', np.dtype('float64'), {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_shape_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'shape', 'size',
     'n', 'best_ns', 'median_ns', 'ns_per_element'])


def nested_list_shaped(shape, leaf=int):
    """leaf is kept exact-typed (float(i) for a float[]/double[] target)
    for consistency with the jpype/jep versions of this file -- jpy's
    own per-element recursion has no exact-type fast path to miss the
    way jpype's ragged-native list-push path does, so a plain int leaf
    would still convert correctly here, it just wouldn't change what's
    being measured across libraries."""
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


for label, dtype, sumfn_by_dims in TYPES:
    leaf = float if label in ('float', 'double') else int

    print(f"=== jpy: list->array, shape sweep, 2D, push (Python -> Java), {label} ===")
    for shape in SHAPES_2D:
        lst = nested_list_shaped(shape, leaf)
        sumfn = sumfn_by_dims[2]
        run(f"list->array {label}[{shape[0]}][{shape[1]}]",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

    print(f"=== jpy: buffer->array, shape sweep, 2D, push (Python -> Java), {label} ===")
    for shape in SHAPES_2D:
        arr = np.arange(total_elements(shape), dtype=dtype).reshape(shape)
        sumfn = sumfn_by_dims[2]
        run(f"buffer->array {label}[{shape[0]}][{shape[1]}]",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), shape, 'buffer', label)

    print(f"=== jpy: list->array, shape sweep, 3D, push (Python -> Java), {label} ===")
    for shape in SHAPES_3D:
        lst = nested_list_shaped(shape, leaf)
        sumfn = sumfn_by_dims[3]
        run(f"list->array {label}[{shape[0]}][{shape[1]}][{shape[2]}]",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), shape, 'list', label)

    print(f"=== jpy: buffer->array, shape sweep, 3D, push (Python -> Java), {label} ===")
    for shape in SHAPES_3D:
        arr = np.arange(total_elements(shape), dtype=dtype).reshape(shape)
        sumfn = sumfn_by_dims[3]
        run(f"buffer->array {label}[{shape[0]}][{shape[1]}][{shape[2]}]",
            lambda arr=arr, sumfn=sumfn: sumfn(arr), shape, 'buffer', label)

csv_log.close()
