""""method dispatch" category, pyjnius side: overload resolution across
16 candidates, monomorphic and polymorphic call sites. Companion:
jpype/dispatch.py, jpy/dispatch.py, jep/dispatch.py -- same operations,
using the shared jpype.benchmark.DeepBench test class. See ../README.md.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/dispatch.py \
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
T0 = autoclass('jpype.benchmark.DeepBench$T0')
T15 = autoclass('jpype.benchmark.DeepBench$T15')

t0 = T0()
t15 = T15()

state = {'flip': False}


def overload_monomorphic():
    return DeepBench.call(t15)


def overload_polymorphic():
    state['flip'] = not state['flip']
    return DeepBench.call(t0 if state['flip'] else t15)


print("=== pyjnius: method dispatch ===")
for name, fn in (
        ("overload x16, monomorphic", overload_monomorphic),
        ("overload x16, polymorphic", overload_polymorphic),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
