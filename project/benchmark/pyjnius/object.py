""""object" category, pyjnius side: plain Object identity (argument +
return value). Companion: jpype/object.py, jpy/object.py, jep/object.py --
same operation, using the shared jpype.benchmark.DeepBench test class.
See ../README.md.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/object.py \
        [classes_dir] [harness_dir]
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jnius_config

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'
jnius_config.set_classpath(classes_dir, harness_dir)

from jnius import autoclass

DeepBench = autoclass('jpype.benchmark.DeepBench')
JObject = autoclass('java.lang.Object')

obj = JObject()


def object_identity():
    return DeepBench.identity(obj)


print("=== pyjnius: object ===")
best, median = timeit(object_identity)
print(format_row("Object identity", best, median))
