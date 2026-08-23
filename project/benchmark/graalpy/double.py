""""double" category, GraalPy side: Math.sqrt(double) and new
Double(double). Companion: jpype/double.py, jpy/double.py, jep/double.py --
same two operations. See ../README.md.

Usage: run via the Bench launcher (see ../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import java

Math = java.type('java.lang.Math')
Double = java.type('java.lang.Double')

i = 0


def math_sqrt():
    global i
    i += 1
    return Math.sqrt(float(i))


def box_double():
    global i
    i += 1
    return Double(float(i))


print("=== GraalPy: double ===")
for name, fn in (
        ("Math.sqrt(double)", math_sqrt),
        ("new Double(double)", box_double),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
