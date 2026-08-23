""""int" category, GraalPy side: Math.max(int,int) and new Integer(int).
Companion: jpype/int.py, jpy/int.py, jep/int.py -- same two operations.
See ../README.md.

Usage: run via the Bench launcher (see ../README.md for the exact
classpath):
    java -cp target/classes:target/lib/* org.jpype.bench.graalpy.Bench \
        project/benchmark/graalpy/int.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import java

Math = java.type('java.lang.Math')
Integer = java.type('java.lang.Integer')

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


print("=== GraalPy: int ===")
for name, fn in (
        ("Math.max(int,int)", math_max),
        ("new Integer(int)", box_integer),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
