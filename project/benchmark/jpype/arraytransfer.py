"""Bulk array-transfer primitives: JArray.pullTo()/pushFrom() (both
1-D primitive arrays only -- dest/src need only match total element
count, not shape), pushFrom's byte-swapped/float16 converting fast path,
direct-buffer sharing, zero-copy slicing, and 2D bulk transfer via
collectRectangular.

Adapted from `reverse`'s benchmark/arraybench/ (bench_array.py's four
models), which drove these from Java through reverse's Java-to-Python
bridge (MainInterpreter/Script) -- that bridge doesn't exist on this
branch, so this version drives the same models from Python directly with
this directory's own timeit/format_row convention instead. Same
comparisons, different driver.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/arraytransfer.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import numpy as np
from jpype import startJVM, shutdownJVM, JClass, JArray, JDouble

startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = JClass('jpype.benchmark.DeepBench')

SIZES = [1_000, 100_000, 1_000_000]


def calls_for(total_elements):
    """Same scaling rule as array_flat.py -- keeps total elements moved
    per benchmark roughly bounded so 1,000,000-element rows don't take
    minutes, especially for the pure-Python-loop comparators below."""
    n = max(10, 2_500_000 // total_elements)
    warmup = max(3, n // 8)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


# ---- Model 1: pullTo (bulk-copy fast path) vs naive per-element walk ----

print("=== JPype: pullTo bulk-copy vs naive per-element pull ===")
for size in SIZES:
    values = np.random.random(size)
    ja = JArray(JDouble)(values.tolist())
    dest = np.empty(size, dtype=np.float64)

    def copy_into(ja=ja, dest=dest):
        ja.pullTo(dest)
        return dest[0]
    run(f"pullTo double[{size}]", copy_into, size)

    # Naive comparator: the only route available before pullTo existed
    # -- element-by-element access through the generic array wrapper.
    # Capped at 100_000: at 1,000,000 elements this is minutes long on
    # its own and isn't the interesting comparison (pullTo's whole
    # point is to avoid this loop).
    if size <= 100_000:
        def naive_sum(ja=ja):
            total = 0.0
            for v in ja:
                total += v
            return total
        run(f"naive per-element double[{size}]", naive_sum, size)

# ---- Model 1a: pullTo, multi-dimensional (int[][]..int[][][][][], N-D
# pullToRectangular) vs the only prior alternative for filling an
# existing destination in place -- there was no N-D pullTo at all before
# this fix (TypeError, "pullTo requires a primitive array"), so the
# comparator here is the same nested-loop walk Model 4 below uses for its
# looped 2D read comparator, generalized to depth via recursion. ----

print("=== JPype: pullTo, multi-dimensional (10^dims elements) ===")
MULTIDIM_DIMS = [2, 3, 4, 5]
MAKE_BY_DIMS = {
    2: DeepBench.make2DIntArray, 3: DeepBench.make3DIntArray,
    4: DeepBench.make4DIntArray, 5: DeepBench.make5DIntArray,
}


def fill_looped(ja, dest):
    if dest.ndim == 1:
        for i in range(len(ja)):
            dest[i] = ja[i]
    else:
        for i in range(len(ja)):
            fill_looped(ja[i], dest[i])


for dims in MULTIDIM_DIMS:
    size = 10 ** dims
    ja = MAKE_BY_DIMS[dims](10)
    dest = np.empty((10,) * dims, dtype=np.int32)

    def pullTo_nd(ja=ja, dest=dest):
        ja.pullTo(dest)
        return dest.flat[0]
    run(f"pullTo int{'[]' * dims}(10^{dims})", pullTo_nd, size)

    # Capped: the whole point of pullTo's N-D path is to avoid this loop,
    # and it grows very slow at higher depth/size.
    if size <= 10_000:
        def naive_fill_nd(ja=ja, dest=dest):
            fill_looped(ja, dest)
            return dest.flat[0]
        run(f"naive per-element int{'[]' * dims}(10^{dims})", naive_fill_nd, size)

# ---- Model 1b: pushFrom (bulk-copy fast path) vs naive per-element fill ----

print("=== JPype: pushFrom bulk-copy vs naive per-element push ===")
for size in SIZES:
    values = np.random.random(size)
    dest = JArray(JDouble)(size)

    def push_into(dest=dest, values=values):
        dest.pushFrom(values)
        return dest[0]
    run(f"pushFrom double[{size}]", push_into, size)

    # Naive comparator: the only route available before pushFrom existed
    # -- element-by-element assignment through the generic array wrapper.
    # Capped at 100_000, same reasoning as Model 1's naive comparator.
    if size <= 100_000:
        def naive_fill(dest=dest, values=values):
            for i in range(size):
                dest[i] = values[i]
            return dest[0]
        run(f"naive per-element double[{size}]", naive_fill, size)

# ---- Model 1b-nd: pushFrom, multi-dimensional (int[][]..int[][][][][],
# N-D pushFromRectangular) vs the only prior alternative -- there was no
# N-D pushFrom at all before this fix (TypeError, "pushFrom requires a
# primitive array"), so the comparator is a recursive per-element
# assignment loop, the mirror of Model 1a's naive pullTo comparator. ----

print("=== JPype: pushFrom, multi-dimensional (10^dims elements) ===")


def push_looped(ja, src):
    if src.ndim == 1:
        for i in range(len(ja)):
            ja[i] = int(src[i])
    else:
        for i in range(len(ja)):
            push_looped(ja[i], src[i])


for dims in MULTIDIM_DIMS:
    size = 10 ** dims
    ja = MAKE_BY_DIMS[dims](10)
    src = np.arange(size, dtype=np.int32).reshape((10,) * dims)

    def pushFrom_nd(ja=ja, src=src):
        ja.pushFrom(src)
        return ja
    run(f"pushFrom int{'[]' * dims}(10^{dims})", pushFrom_nd, size)

    # Capped, same reasoning as Model 1a's naive comparator.
    if size <= 10_000:
        def naive_push_nd(ja=ja, src=src):
            push_looped(ja, src)
            return ja
        run(f"naive per-element int{'[]' * dims}(10^{dims})", naive_push_nd, size)

# ---- Model 1c: pushFrom converting fast path ----
# Non-native-byte-order and float16 sources used to fall all the way back
# to a scalar converter()/pack() loop, one GetPrimitiveArrayCritical pair
# per call, same as the dtype-matching path had before pushFrom existed.
# The bulk fast path now covers both in a single JNI crossing -- compare
# directly against the matching-dtype row above.

print("=== JPype: pushFrom converting fast path (byte-swapped / float16) ===")
for size in SIZES:
    dest = JArray(JDouble)(size)
    byteswapped = np.random.random(size).astype('>f8')

    def push_byteswapped(dest=dest, byteswapped=byteswapped):
        dest.pushFrom(byteswapped)
        return dest[0]
    run(f"pushFrom byteswapped double[{size}]", push_byteswapped, size)

    float16_src = np.random.random(size).astype(np.float16)

    def push_float16(dest=dest, float16_src=float16_src):
        dest.pushFrom(float16_src)
        return dest[0]
    run(f"pushFrom float16 double[{size}]", push_float16, size)

# ---- Model 2: direct-buffer-shared (steady-state zero-copy) ----

print("=== JPype: direct java.nio.DoubleBuffer -> numpy, steady-state ===")
ByteBuffer = JClass('java.nio.ByteBuffer')
for size in SIZES:
    bb = ByteBuffer.allocateDirect(size * 8)
    buf = bb.asDoubleBuffer()
    for i in range(size):
        buf.put(i, float(i))
    # Wrap once outside the timed loop -- the fair comparison is the cost
    # of *sharing* the memory on each access, not wrapper construction.
    arr = np.asarray(buf)

    def sum_direct_buffer_shared(arr=arr):
        return float(arr.sum())
    run(f"direct-buffer-shared double[{size}]", sum_direct_buffer_shared, size)

# ---- Model 3: slicing (zero-copy view vs pullTo on a Java-array slice) ----

print("=== JPype: slicing, Python view vs Java-array-slice pullTo ===")
for size, step in zip(SIZES, [2, 2, 2]):
    src = np.random.random(size)

    def sum_slice(src=src, step=step):
        return float(src[::step].sum())
    run(f"slice_python double[{size}]", sum_slice, size)

    values = np.random.random(size)
    ja = JArray(JDouble)(values.tolist())

    def sum_java_array_slice(ja=ja, step=step):
        sliced = ja[::step]
        dest = np.empty(len(sliced), dtype=np.float64)
        sliced.pullTo(dest)
        return float(dest.sum())
    run(f"slice_javaArray double[{size}]", sum_java_array_slice, size)

# ---- Model 4: multidimensional bulk transfer (real double[][]) ----

print("=== JPype: 2D bulk transfer, np.asarray(collectRectangular) vs Python loop ===")
MAT_SHAPES = [(10, 10), (300, 300), (1000, 1000)]
for rows, cols in MAT_SHAPES:
    total = rows * cols
    mat = JArray(JDouble, 2)(rows)
    for r in range(rows):
        mat[r] = JArray(JDouble)([float(r * cols + c) for c in range(cols)])

    def sum_2d_bulk(mat=mat):
        return float(np.asarray(mat).sum())
    run(f"multidim_bulk {rows}x{cols}", sum_2d_bulk, total)

    # Python-level per-row loop -- the only route for a bridge with no
    # multidim accelerator (mirrors jpy's/jep's approach). Capped: the
    # 1000x1000 case is the whole point of the bulk path existing, not
    # a case we need the slow comparator's exact number for.
    if total <= 90_000:
        def sum_2d_looped(mat=mat):
            total_ = 0.0
            for row in mat:
                total_ += sum(row)
            return total_
        run(f"multidim_looped {rows}x{cols}", sum_2d_looped, total)

shutdownJVM()
