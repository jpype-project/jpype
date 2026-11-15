"""JArray.toList()'s dtype argument: plain-Python default vs. explicit
forced-cast wrapping/casting, flat 1D arrays at increasing sizes.

toList() used to always box every element as a tagged wrapper instance
(JInt/JDouble/etc, via convertToPythonObject -- tp_alloc + Java-slot
assignment). It now defaults to plain Python int/float/bool/str (a bare
PyLong_From*/PyFloat_FromDouble/PyBool_FromLong/PyUnicode_FromOrdinal,
no wrapper allocation, no Java-slot tagging) and only pays the wrapper
cost when a caller explicitly asks for it via dtype=JInt/JDouble/etc.

Categories per type, all reading the same freshly-built array:
  - toList() [[plain, new default]]: bare Python values, no wrapper.
  - toList(dtype=<same type>) [[wrapped, ~= old default]]: identity cast,
    but boxed as a tagged wrapper instance -- reproduces exactly what
    every toList() call used to cost before this change, so this row is
    the "before" number.
  - toList(dtype=int) / toList(dtype=float): forced numeric cast, plain
    output -- the interesting new case for int[]/long[] read as float or
    float[]/double[] read as int.
  - list(arr): the pre-toList() naive per-element baseline, for scale.

See ../array_flat.py (the general flat-array benchmark, whose own
array->list.toList() row now reflects the new plain default automatically)
and ../README.md.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/array_to_list_dtype.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

from jpype import startJVM, shutdownJVM, JClass, JInt, JLong, JFloat, JDouble

startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = JClass('jpype.benchmark.DeepBench')

SIZES = [100, 1_000, 10_000, 100_000]

# (label, own JType, crossType, makeArray)
TYPES = [
    ('int', JInt, float, DeepBench.makeIntArray),
    ('long', JLong, float, DeepBench.makeLongArray),
    ('float', JFloat, int, DeepBench.makeFloatArray),
    ('double', JDouble, int, DeepBench.makeDoubleArray),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_to_list_dtype_results.csv'),
    ['category', 'dtype', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, category, dtype):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category=category, dtype=dtype, size=total_elements,
                   n=n, best_ns=best, median_ns=median)


for label, jtype, crossType, makefn in TYPES:
    print(f"=== JPype: toList() plain vs wrapped vs forced-cast, flat, {label} ===")
    for size in SIZES:
        run(f"list(arr) {label}[{size}]",
            lambda size=size, makefn=makefn: list(makefn(size)), size,
            'list_naive', label)

        run(f"toList() {label}[{size}], plain",
            lambda size=size, makefn=makefn: makefn(size).toList(), size,
            'toList_plain', label)

        run(f"toList(dtype={label}) {label}[{size}], wrapped (~= old default)",
            lambda size=size, makefn=makefn, jtype=jtype: makefn(size).toList(dtype=jtype), size,
            'toList_wrapped_identity', label)

        crossLabel = 'float' if crossType is float else 'int'
        run(f"toList(dtype={crossLabel}) {label}[{size}], forced cast, plain",
            lambda size=size, makefn=makefn, crossType=crossType: makefn(size).toList(dtype=crossType), size,
            'toList_forced_cast_plain', label)

csv_log.close()
shutdownJVM()
