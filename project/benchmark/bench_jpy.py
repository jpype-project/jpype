"""General per-call overhead benchmark, jpy side -- same operations as
bench_jpype.py, for a direct comparison. See README.md.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/bench_jpy.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from _common import timeit, format_row

import jpyutil

jpyutil.init_jvm(jvm_maxmem='512M')
import jpy

Math = jpy.get_type('java.lang.Math')
Integer = jpy.get_type('java.lang.Integer')
String = jpy.get_type('java.lang.String')

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


def string_roundtrip():
    s = String("hello")
    return str(s)


print("=== jpy ===")
for name, fn in (
        ("Math.max(int,int)", math_max),
        ("new Integer(int)", box_integer),
        ("new String + toString", string_roundtrip),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
