""""method dispatch" category, GraalPy side: overload resolution across 16
candidates, monomorphic and polymorphic call sites. See ../README.md.

Usage: run via the Bench launcher with DeepBench on the classpath (see
../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import java

DeepBench = java.type('jpype.benchmark.DeepBench')
T0 = java.type('jpype.benchmark.DeepBench$T0')
T15 = java.type('jpype.benchmark.DeepBench$T15')

t0 = T0()
t15 = T15()

state = {'flip': False}


def overload_monomorphic():
    return DeepBench.call(t15)


def overload_polymorphic():
    state['flip'] = not state['flip']
    return DeepBench.call(t0 if state['flip'] else t15)


print("=== GraalPy: method dispatch ===")
for name, fn in (
        ("overload x16, monomorphic", overload_monomorphic),
        ("overload x16, polymorphic", overload_polymorphic),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
