"""JArray.of() -- constructing a jpype array directly from a buffer-protocol
source (typically numpy), at increasing flat sizes and at fixed-total-element
multi-dimensional depths. No equivalent in the jpy/jep/pyjnius suites (this
is jpype-only API surface), so this is jpype-only, `best` ns/call.

Why this needs its own script rather than reusing array_flat.py/
array_multidim.py's existing "buffer->array" rows: `JArray.of(arr)` and
`DeepBench.void{Type}Array(arr)` (the method-argument buffer push measured
there) look like the same operation -- "push a numpy array into a Java
primitive array" -- but they go through genuinely different native code
paths:

  - Method-argument push, flat (1D): `JPConversionBuffer::convert`
    (jp_classhints.cpp) hands the whole source buffer to
    `Support.fillFlatFromBuffer` in one JNI call -- a real bulk
    typed-buffer copy on the Java side, no per-element work at all for the
    common matching-dtype case.
  - `JArray.of(arr)`, at *any* dimensionality including flat 1D: always
    goes through `PyJPModule_convertBuffer` -> `JPPrimitiveType::newMultiArray`
    -> `convertMultiArrayObject` (jp_primitive_accessor.h) -- the
    N-dimensional buffer-push machinery, shared with the method-argument
    push path's own N>=2 case (Section 5's "buffer->array" rows there).
    That path has no bulk-copy shortcut for the matching-dtype case: it
    calls `pack(dest, converter(src))` once per element inside the
    traversal loop, unconditionally, even when converter is really just
    an identity cast. It predates the flat-path optimization
    (fillFlatFromBuffer) and was never given the same treatment.

So a flat `JArray.of(np.arange(100_000, dtype=np.int32))` is expected to
cost close to Section 5's N-D `buffer->array` numbers (per-element pack
loop), not Section 3's flat `buffer->array` numbers (bulk JNI handoff) --
even though both are "push a matching-dtype numpy array into an int[]".
This script measures that gap directly instead of leaving it to be
inferred from two different tables.

Categories:
  - "JArray.of(arr)": dtype auto-detected from the buffer's own format.
  - "JArray.of(arr, dtype=JType)": explicit *matching* dtype -- same
    conversion cost as auto-detect, isolates the dtype-lookup branch's own
    overhead (parseDtypeArg/PyJPClass_getJPClass) from the auto-detect
    format-switch branch.
  - "JArray.of(arr, dtype=<cross type>)": explicit dtype requiring a real
    per-element cast (e.g. an int64 source read as JInt) -- the case
    `converter` is actually doing non-identity work in, for comparison
    against the matching-dtype rows above.
  - "JArray(JType)(arr)": the naive alternative a user might reach for
    instead of `.of()` -- numpy arrays satisfy `PySequence_Check`, so this
    goes through `PyJPArray_init`'s generic sequence branch
    (`JPArray::setRange`), not the buffer protocol at all. Included to
    quantify the trap: this is *not* a buffer-protocol path, so it pays
    Python-level per-element sequence access on top of not having a bulk
    copy either. Flat (1D) only -- see "JArray(JType, dims)(arr)" below
    for its multi-dimensional counterpart.
  - "JArray(JType, dims)(arr)": the manual type+dims constructor spelling
    (equivalently `JType[:, :, ...](arr)`), multi-dimensional only (dims
    >= 2). Unlike the flat naive-ctor row above, this *does* take the
    buffer-protocol fast path (`PyJPArray_init`'s buffer check, gated the
    same way as `JArray.of()`'s own N-D branch) for a matching-dtype
    contiguous source, so it's expected to track "JArray.of(arr)" closely
    rather than the naive row -- included to confirm the manual spelling
    isn't leaving performance on the table relative to `.of()`.

Writes project/benchmark/jpype/array_of_results.csv alongside the printed
output.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/array_of.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import numpy as np
from jpype import startJVM, shutdownJVM, JArray, JInt, JLong, JFloat, JDouble

startJVM(classpath=['test/classes', 'test/harness'])

SIZES = [100, 1_000, 10_000, 100_000]
DIMS = [2, 3, 4, 5]

# (label, matching numpy dtype, cross numpy dtype (same itemsize family,
# different kind, to force a real converter call), JType, array ndims-1 ctor)
TYPES = [
    ('int', np.dtype('int32'), np.dtype('float32'), JInt),
    ('long', np.dtype('int64'), np.dtype('float64'), JLong),
    ('float', np.dtype('float32'), np.dtype('int32'), JFloat),
    ('double', np.dtype('float64'), np.dtype('int64'), JDouble),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_of_results.csv'),
    ['category', 'dims', 'dtype', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, category, dtype, dims):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category=category, dims=dims, dtype=dtype, size=total_elements,
                   n=n, best_ns=best, median_ns=median)


for label, dtype, cross_dtype, jtype in TYPES:
    print(f"=== JPype: JArray.of(), flat, {label} ===")
    for size in SIZES:
        arr = np.arange(size, dtype=dtype)
        run(f"JArray.of(arr) {label}[{size}]",
            lambda arr=arr: JArray.of(arr), size, 'of_auto', label, 1)

        run(f"JArray.of(arr, dtype={label}) {label}[{size}], matching",
            lambda arr=arr, jtype=jtype: JArray.of(arr, dtype=jtype), size,
            'of_dtype_matching', label, 1)

        cross_arr = np.arange(size, dtype=cross_dtype)
        run(f"JArray.of(arr, dtype={label}) {label}[{size}], cross-dtype cast",
            lambda cross_arr=cross_arr, jtype=jtype: JArray.of(cross_arr, dtype=jtype), size,
            'of_dtype_cross', label, 1)

        run(f"JArray({label})(arr) {label}[{size}], naive sequence ctor",
            lambda arr=arr, jtype=jtype: JArray(jtype)(arr), size,
            'naive_sequence_ctor', label, 1)

    print(f"=== JPype: JArray.of(), multi-dimensional (10^dims elements), {label} ===")
    for dims in DIMS:
        size = 10 ** dims
        arr = np.arange(size, dtype=dtype).reshape((10,) * dims)
        run(f"JArray.of(arr) {label}{'[]' * dims}(10^{dims})",
            lambda arr=arr: JArray.of(arr), size, 'of_auto', label, dims)

        run(f"JArray.of(arr, dtype={label}) {label}{'[]' * dims}(10^{dims}), matching",
            lambda arr=arr, jtype=jtype: JArray.of(arr, dtype=jtype), size,
            'of_dtype_matching', label, dims)

        run(f"JArray({label}, {dims})(arr) {label}{'[]' * dims}(10^{dims}), manual ctor",
            lambda arr=arr, jtype=jtype, dims=dims: JArray(jtype, dims)(arr), size,
            'manual_ctor_nd', label, dims)

csv_log.close()
shutdownJVM()
