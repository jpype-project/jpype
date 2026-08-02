""""string" category, jpy side: new String(...) + .toString(). Companion:
jpype/strings.py, jep/strings.py -- same operation. See ../README.md.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/strings.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpyutil

jpyutil.init_jvm(jvm_maxmem='512M')
import jpy

String = jpy.get_type('java.lang.String')


def string_roundtrip():
    s = String("hello")
    return str(s)


print("=== jpy: string ===")
best, median = timeit(string_roundtrip)
print(format_row("new String + toString", best, median))
