""""int" category, JPype side: Math.max(int,int) and new Integer(int).
Companion: jpy/int.py, jep/int.py -- same two operations, for a direct
comparison. See ../README.md.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/int.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpype

jpype.startJVM()

Math = jpype.JClass('java.lang.Math')
Integer = jpype.JClass('java.lang.Integer')

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


print("=== JPype: int ===")
for name, fn in (
        ("Math.max(int,int)", math_max),
        ("new Integer(int)", box_integer),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))

jpype.shutdownJVM()
