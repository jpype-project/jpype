""""proxy" category, GraalPy side: Java calling back into Python through an
established callback binding -- both a primitive `int` argument and an
`Object` argument (including a genuinely null one). Companion:
jpype/proxy.py, jep/proxy.py -- same operations, using the shared
jpype.benchmark.DeepBench test class. No jpy/proxy.py: see ../README.md.

Unlike jpype/jep/pyjnius, GraalPy's host-Java-calls-guest-Python direction
needs no explicit proxy-construction API at all -- a plain Python object
(or even a bare function, for a single-method interface) with a matching
method name is accepted anywhere a Java functional interface is expected,
auto-adapted by GraalPy's polyglot layer on the call boundary. Confirmed
empirically: DeepBench.invokeObjectCallback(cb, obj) and
invokeObjectCallbackWithNull(cb) (the null-argument case that crashes
pyjnius, see ../README.md) both work with no special handling.

Usage: run via the Bench launcher with DeepBench on the classpath (see
../README.md).
"""
import sys
import os

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import java

DeepBench = java.type('jpype.benchmark.DeepBench')


class MyCallback:
    def run(self, x):
        return x + 1


proxy = MyCallback()


def proxy_callback():
    return DeepBench.invokeCallback(proxy, 5)


class MyObjCallback:
    def handle(self, o):
        return o


obj_proxy = MyObjCallback()
callback_arg = java.type('java.lang.Object')()


def proxy_object_arg():
    return DeepBench.invokeObjectCallback(obj_proxy, callback_arg)


print("=== GraalPy: proxy ===")
for name, fn in (
        ("proxy callback (established), int arg", proxy_callback),
        ("proxy callback (established), Object arg", proxy_object_arg),
):
    best, median = timeit(fn)
    print(format_row(name, best, median))
