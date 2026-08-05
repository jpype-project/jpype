"""General per-call overhead benchmark, JPype side.

Companion scripts (bench_jpy.py, bench_jep.py) perform the identical
operations through jpy and jep so the numbers are comparable. See
README.md in this directory for how to run all three.

Usage:
    /path/to/venv/bin/python project/benchmark/bench_jpype.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from _common import timeit, format_row

import jpype

jpype.startJVM()

Math = jpype.JClass('java.lang.Math')
Integer = jpype.JClass('java.lang.Integer')
Double = jpype.JClass('java.lang.Double')
String = jpype.JClass('java.lang.String')

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


def math_sqrt():
    global i
    i += 1
    return Math.sqrt(float(i))


def box_double():
    global i
    i += 1
    return Double(float(i))


def string_roundtrip():
    s = String("hello")
    return str(s)


print("=== JPype ===")
for name, fn in (
        ("Math.max(int,int)", math_max),
        ("new Integer(int)", box_integer),
        ("Math.sqrt(double)", math_sqrt),
        ("new Double(double)", box_double),
        ("new String + toString", string_roundtrip),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))

jpype.shutdownJVM()
