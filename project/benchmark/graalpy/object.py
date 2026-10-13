""""object" category, GraalPy side: plain Object identity (argument +
return value). See ../README.md.

Usage: run via the Bench launcher with DeepBench on the classpath (see
../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import java

DeepBench = java.type('jpype.benchmark.DeepBench')

obj = java.type('java.lang.Object')()


def object_identity():
    return DeepBench.identity(obj)


print("=== GraalPy: object ===")
best, median = timeit(object_identity)
print(format_row("Object identity", best, median))
