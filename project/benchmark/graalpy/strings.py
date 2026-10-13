""""string" category, GraalPy side: new String(...) + .toString(). See
../README.md.

Usage: run via the Bench launcher (see ../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import java

String = java.type('java.lang.String')


def string_roundtrip():
    s = String("hello")
    return str(s)


print("=== GraalPy: string ===")
best, median = timeit(string_roundtrip)
print(format_row("new String + toString", best, median))
