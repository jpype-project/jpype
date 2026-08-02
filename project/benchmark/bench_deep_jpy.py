"""Deeper conversion-chain benchmark, jpy side -- same operations as
bench_deep_jpype.py. See README.md.

Usage:
    /path/to/jpy-venv/bin/python project/benchmark/bench_deep_jpy.py \
        /path/to/jpype/test/classes /path/to/jpype/test/harness
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from _common import timeit, format_row

import numpy as np
import jpyutil

classes_dir = sys.argv[1] if len(sys.argv) > 1 else 'test/classes'
harness_dir = sys.argv[2] if len(sys.argv) > 2 else 'test/harness'

jpyutil.init_jvm(jvm_maxmem='512M', jvm_classpath=[classes_dir, harness_dir])
import jpy

DeepBench = jpy.get_type('jpype.benchmark.DeepBench')
T0 = jpy.get_type('jpype.benchmark.DeepBench$T0')
T15 = jpy.get_type('jpype.benchmark.DeepBench$T15')
JObject = jpy.get_type('java.lang.Object')

t0 = T0()
t15 = T15()

state = {'flip': False}


def overload_monomorphic():
    return DeepBench.call(t15)


def overload_polymorphic():
    state['flip'] = not state['flip']
    return DeepBench.call(t0 if state['flip'] else t15)


ARRAY = list(range(100))


def array_arg():
    return DeepBench.sumIntArray(ARRAY)


NUMPY_ARRAY = np.arange(100, dtype=np.int32)


def array_arg_buffer():
    return DeepBench.sumIntArray(NUMPY_ARRAY)


NESTED_LIST = [list(range(10)) for _ in range(10)]


def array_arg_2d_list():
    return DeepBench.sum2DIntArray(NESTED_LIST)


NUMPY_2D = np.arange(100, dtype=np.int32).reshape(10, 10)


def array_arg_2d_buffer():
    return DeepBench.sum2DIntArray(NUMPY_2D)


obj = JObject()


def object_identity():
    return DeepBench.identity(obj)


# No "proxy callback" entry here: jpy's PyObject.createProxy() mechanism
# (the only one found for exposing a Python object as a Java interface)
# didn't work from Python in this checkout -- the object it returns isn't
# recognized as its interface type when passed to any Java method
# ("no matching Java method overloads found"), even mirroring jpy's own
# ReachabilityFenceTestFixture.stressProxy test pattern exactly. See
# README.md.

print("=== jpy: deep conversion paths ===")
for name, fn in (
        ("overload x16, monomorphic", overload_monomorphic),
        ("overload x16, polymorphic", overload_polymorphic),
        ("int[] from list(100), fresh", array_arg),
        ("int[] from numpy(100), fresh", array_arg_buffer),
        ("int[][] from nested list(10x10), fresh", array_arg_2d_list),
        ("int[][] from numpy(10x10), fresh", array_arg_2d_buffer),
        ("Object identity", object_identity),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
