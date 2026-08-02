"""Deeper conversion-chain benchmark, JPype side. Companion to
bench_jpype.py, which only exercises trivial single-overload JDK calls.
This one hits paths bench_jpype.py doesn't:

  - overload resolution across 16 candidates (JPMethodDispatch::
    findOverload), both a monomorphic call site (repeated same type,
    should hit its existing single-slot m_LastOverload cache) and a
    polymorphic one (alternating types, defeats that cache every call)
  - array-argument conversion from a plain Python list (JPConversionSequence
    in jp_classhints.cpp), which is inherently content-dependent and so
    was NOT made cacheable by this session's JPClass::findJavaConversion
    work -- every call re-walks the list.

See README.md for jpy/jep equivalents and how to run all three together.

Usage:
    /path/to/venv/bin/python project/benchmark/bench_deep_jpype.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from _common import timeit, format_row

import numpy as np
import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')
T0 = jpype.JClass('jpype.benchmark.DeepBench$T0')
T15 = jpype.JClass('jpype.benchmark.DeepBench$T15')
Callback = jpype.JClass('jpype.benchmark.DeepBench$Callback')
ObjectCallback = jpype.JClass('jpype.benchmark.DeepBench$ObjectCallback')

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


# Buffer-protocol transfer -- a numpy int32 array, unlike a plain list,
# is handled by setArrayRange's PyObject_CheckBuffer branch (direct
# memory copy via a jconverter, no per-element JPMatch/findJavaConversion
# at all for the matches()-side quality check either, since
# JPConversionBuffer -- unlike JPConversionSequence -- only inspects
# element 0). See project/benchmark/README.md.
NUMPY_ARRAY = np.arange(100, dtype=np.int32)


def array_arg_buffer():
    return DeepBench.sumIntArray(NUMPY_ARRAY)


NESTED_LIST = [list(range(10)) for _ in range(10)]


def array_arg_2d_list():
    return DeepBench.sum2DIntArray(NESTED_LIST)


NUMPY_2D = np.arange(100, dtype=np.int32).reshape(10, 10)


def array_arg_2d_buffer():
    return DeepBench.sum2DIntArray(NUMPY_2D)


obj = jpype.JObject()


def object_identity():
    return DeepBench.identity(obj)


# proxy: an @JImplements object constructed once (steady-state established
# binding), not a bare Python function (which JPFunctional re-wraps fresh
# on every conversion) -- see project/benchmark/README.md.
@jpype.JImplements(Callback)
class MyCallback:
    @jpype.JOverride
    def run(self, x):
        return x + 1


proxy = MyCallback()


def proxy_callback():
    return DeepBench.invokeCallback(proxy, 5)


# Same idea, but with an Object-typed argument -- exercises jp_proxy.cpp's
# getArgs() (its own findClassForObject call, separate from
# JPClass::convertToPythonObject), unlike proxy_callback above (a
# primitive int arg never goes through that code at all).
@jpype.JImplements(ObjectCallback)
class MyObjCallback:
    @jpype.JOverride
    def handle(self, o):
        return o


obj_proxy = MyObjCallback()
callback_arg = jpype.JObject()


def proxy_object_arg():
    return DeepBench.invokeObjectCallback(obj_proxy, callback_arg)


print("=== JPype: deep conversion paths ===")
for name, fn in (
        ("overload x16, monomorphic", overload_monomorphic),
        ("overload x16, polymorphic", overload_polymorphic),
        ("int[] from list(100), fresh", array_arg),
        ("int[] from numpy(100), fresh", array_arg_buffer),
        ("int[][] from nested list(10x10), fresh", array_arg_2d_list),
        ("int[][] from numpy(10x10), fresh", array_arg_2d_buffer),
        ("Object identity", object_identity),
        ("proxy callback (established)", proxy_callback),
        ("proxy callback, Object arg", proxy_object_arg),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))

jpype.shutdownJVM()
