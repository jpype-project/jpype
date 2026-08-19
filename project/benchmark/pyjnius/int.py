""""int" category, pyjnius side: Math.max(int,int) and new Integer(int).
Companion: jpype/int.py, jpy/int.py, jep/int.py -- same two operations.
See ../README.md.

pyjnius auto-starts its embedded JVM on first `autoclass()` call (using
whatever classpath was set via `jnius_config.set_classpath()` beforehand)
and needs no explicit shutdown call -- the JVM is torn down with the
process.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/int.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

from jnius import autoclass

Math = autoclass('java.lang.Math')
Integer = autoclass('java.lang.Integer')

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


print("=== pyjnius: int ===")
for name, fn in (
        ("Math.max(int,int)", math_max),
        ("new Integer(int)", box_integer),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
