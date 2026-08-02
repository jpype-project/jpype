""""string" category, JPype side: new String(...) + .toString(). Companion:
jpy/strings.py, jep/strings.py -- same operation. See ../README.md.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/strings.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpype

jpype.startJVM()

String = jpype.JClass('java.lang.String')


def string_roundtrip():
    s = String("hello")
    return str(s)


print("=== JPype: string ===")
best, median = timeit(string_roundtrip)
print(format_row("new String + toString", best, median))

jpype.shutdownJVM()
