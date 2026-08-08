""""proxy" category, pyjnius side: Java calling back into Python through
an established callback binding (a `PythonJavaClass` subclass with a
`@java_method` implementation, constructed once). Companion:
jpype/proxy.py, jep/proxy.py -- but int arg *only*, not Object arg -- see
below. No jpy/proxy.py either (see ../README.md for that gap).

Only the `int`-arg case (`invokeCallback`) is benchmarked here.
`invokeObjectCallback`/`invokeObjectCallbackWithNull` are deliberately
NOT called from this file: they reliably crash the JVM with a native
SIGSEGV in this pyjnius checkout, confirmed independently three times
against a fresh build in a disposable venv (not a stale-build artifact --
see this repo's CLAUDE.md for why that was ruled out first):

    #
    # A fatal error has been detected by the Java Runtime Environment:
    #
    #  SIGSEGV (0xb) at pc=0x0000...804, pid=..., tid=...
    #
    # Problematic frame:
    # V  [libjvm.so+0x...804]  jni_GetObjectClass+0xa4

This reproduces even for `invokeObjectCallbackWithNull` alone -- a
Python-implemented Java interface method receiving a genuinely null
`Object` argument. `jpype.benchmark.DeepBench`'s ObjectCallback methods
exist specifically to cover this case (a jp_proxy.cpp regression test in
jpype's own history): calling `GetObjectClass`/`IsSameObject` on a null
jobject is undefined behavior over JNI, and this pyjnius checkout's
proxy-argument-marshalling code does exactly that without a null check.
Even the non-crashing case (`invokeObjectCallback` with a real, non-null
Object argument) doesn't work correctly either -- it silently returns
`None` instead of the object handed back by the Python callback, a
separate (non-fatal) correctness bug in pyjnius's proxy return-value
handling. Neither is a benchmark worth reporting a number for, and the
crash means this file must never call the null-argument variant.

Usage:
    /path/to/pyjnius-venv/bin/python project/benchmark/pyjnius/proxy.py \
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

from jnius import autoclass, java_method, PythonJavaClass

DeepBench = autoclass('jpype.benchmark.DeepBench')


class MyCallback(PythonJavaClass):
    __javainterfaces__ = ['jpype/benchmark/DeepBench$Callback']
    __javacontext__ = 'app'

    @java_method('(I)I')
    def run(self, x):
        return x + 1


proxy = MyCallback()


def proxy_callback():
    return DeepBench.invokeCallback(proxy, 5)


print("=== pyjnius: proxy ===")
best, median = timeit(proxy_callback)
print(format_row("proxy callback (established), int arg", best, median))
