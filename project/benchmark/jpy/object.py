""""object" category, jpy side: plain Object identity (argument + return
value). Companion: jpype/object.py, jep/object.py -- same operation, using
the shared jpype.benchmark.DeepBench test class. See ../README.md.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/jpy/object.py \
        /path/to/jpype/test/classes /path/to/jpype/test/harness
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpyutil

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'

jpyutil.init_jvm(jvm_maxmem='512M', jvm_classpath=[classes_dir, harness_dir])
import jpy

DeepBench = jpy.get_type('jpype.benchmark.DeepBench')
JObject = jpy.get_type('java.lang.Object')

obj = JObject()


def object_identity():
    return DeepBench.identity(obj)


print("=== jpy: object ===")
best, median = timeit(object_identity)
print(format_row("Object identity", best, median))
