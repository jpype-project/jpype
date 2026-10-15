""""string" category, pyjnius side: new String(...) + .toString().
Companion: jpype/strings.py, jpy/strings.py, jep/strings.py -- same
operation. See ../README.md.

Named strings.py, not string.py, everywhere in this suite: a script's
own directory lands on sys.path when it's run directly, so a file named
string.py here would shadow the stdlib `string` module for anything
imported afterward that needs it (this broke `logging`'s import during
development).

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/strings.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

from jnius import autoclass

String = autoclass('java.lang.String')


def string_roundtrip():
    s = String("hello")
    return s.toString()


print("=== pyjnius: string ===")
best, median = timeit(string_roundtrip)
print(format_row("new String + toString", best, median))
