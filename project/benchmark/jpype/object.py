""""object" category, JPype side: plain Object identity (argument + return
value, no conversion beyond wrapping/unwrapping a reference). Companion:
jpy/object.py, jep/object.py -- same operation, using the shared
jpype.benchmark.DeepBench test class
(test/harness/jpype/benchmark/DeepBench.java). See ../README.md.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/object.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

obj = jpype.JObject()


def object_identity():
    return DeepBench.identity(obj)


print("=== JPype: object ===")
best, median = timeit(object_identity)
print(format_row("Object identity", best, median))

jpype.shutdownJVM()
