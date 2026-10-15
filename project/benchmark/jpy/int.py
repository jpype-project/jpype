""""int" category, jpy side: Math.max(int,int) and new Integer(int).
Companion: jpype/int.py, jep/int.py -- same two operations. See
../README.md.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/int.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpyutil

jpyutil.init_jvm(jvm_maxmem='512M')
import jpy

Math = jpy.get_type('java.lang.Math')
Integer = jpy.get_type('java.lang.Integer')

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


print("=== jpy: int ===")
for name, fn in (
        ("Math.max(int,int)", math_max),
        ("new Integer(int)", box_integer),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
