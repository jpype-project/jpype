""""proxy" category, JPype side: Java calling back into Python through an
established callback binding (an @JImplements object constructed once,
not a per-call proxy) -- both a primitive `int` argument and an `Object`
argument (the latter also exercising jp_proxy.cpp's getArgs(), separate
from JPClass::convertToPythonObject). Companion: jep/proxy.py -- same
operations, using the shared jpype.benchmark.DeepBench test class. No
jpy/proxy.py: see ../README.md for why jpy's proxy mechanism didn't work
from Python in this checkout.

Usage:
    /path/to/venv/bin/python project/benchmark/jpype/proxy.py
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')
Callback = jpype.JClass('jpype.benchmark.DeepBench$Callback')
ObjectCallback = jpype.JClass('jpype.benchmark.DeepBench$ObjectCallback')


@jpype.JImplements(Callback)
class MyCallback:
    @jpype.JOverride
    def run(self, x):
        return x + 1


proxy = MyCallback()


def proxy_callback():
    return DeepBench.invokeCallback(proxy, 5)


@jpype.JImplements(ObjectCallback)
class MyObjCallback:
    @jpype.JOverride
    def handle(self, o):
        return o


obj_proxy = MyObjCallback()
callback_arg = jpype.JObject()


def proxy_object_arg():
    return DeepBench.invokeObjectCallback(obj_proxy, callback_arg)


print("=== JPype: proxy ===")
for name, fn in (
        ("proxy callback (established), int arg", proxy_callback),
        ("proxy callback (established), Object arg", proxy_object_arg),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))

jpype.shutdownJVM()
