""""method dispatch" category, JPype side: overload resolution across 16
candidates (JPMethodDispatch::findOverload), both a monomorphic call site
(repeated same type, should hit the existing single-slot m_LastOverload
cache) and a polymorphic one (alternating types, defeats that cache every
call). Companion: jpy/dispatch.py, jep/dispatch.py -- same operations,
using the shared jpype.benchmark.DeepBench test class. See ../README.md.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/dispatch.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')
T0 = jpype.JClass('jpype.benchmark.DeepBench$T0')
T15 = jpype.JClass('jpype.benchmark.DeepBench$T15')

t0 = T0()
t15 = T15()

state = {'flip': False}


def overload_monomorphic():
    return DeepBench.call(t15)


def overload_polymorphic():
    state['flip'] = not state['flip']
    return DeepBench.call(t0 if state['flip'] else t15)


print("=== JPype: method dispatch ===")
for name, fn in (
        ("overload x16, monomorphic", overload_monomorphic),
        ("overload x16, polymorphic", overload_polymorphic),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))

jpype.shutdownJVM()
