""""double" category, jpy side: Math.sqrt(double) and new Double(double).
Companion: jpype/double.py, jep/double.py -- same two operations. See
../README.md.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/double.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpyutil

jpyutil.init_jvm(jvm_maxmem='512M')
import jpy

Math = jpy.get_type('java.lang.Math')
Double = jpy.get_type('java.lang.Double')

i = 0


def math_sqrt():
    global i
    i += 1
    return Math.sqrt(float(i))


def box_double():
    global i
    i += 1
    return Double(float(i))


print("=== jpy: double ===")
for name, fn in (
        ("Math.sqrt(double)", math_sqrt),
        ("new Double(double)", box_double),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
