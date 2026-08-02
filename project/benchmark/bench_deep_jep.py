"""Deeper conversion-chain benchmark, jep side -- same operations as
bench_deep_jpype.py. See README.md.

Like bench_jep.py, this can't sys.path-import _common.py (no __file__ in
jep's embedded execution context) and writes results to a file instead of
stdout.

Usage: see README.md for the classpath (needs jep.jar AND
test/classes + test/harness) / library-path / PYTHONPATH this needs.
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/bench_deep_jep.py <output_path>
"""
import sys
import time


def timeit(fn, n=200_000, warmup=1000, trials=5):
    for _ in range(warmup):
        fn()
    samples = []
    for _ in range(trials):
        t0 = time.perf_counter()
        for _ in range(n):
            fn()
        t1 = time.perf_counter()
        samples.append((t1 - t0) / n * 1e9)
    samples.sort()
    return samples[0], samples[len(samples) // 2]


def format_row(name, best, median):
    return f"{name:32s} best={best:8.1f} ns/call  median={median:8.1f} ns/call"


from jpype.benchmark import DeepBench
from java.lang import Object as JObject
import numpy as np
import jep

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_deep_jep_results.txt'

t0_obj = DeepBench.T0()
t15_obj = DeepBench.T15()

state = {'flip': False}


def overload_monomorphic():
    return DeepBench.call(t15_obj)


def overload_polymorphic():
    state['flip'] = not state['flip']
    return DeepBench.call(t0_obj if state['flip'] else t15_obj)


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


# proxy: jep.jproxy(pyobj, [interfaces]) creates the binding once; the
# Callback interface is the same jpype.benchmark.DeepBench$Callback used
# by bench_deep_jpype.py, so invokeCallback itself is identical.
class MyCallback:
    def run(self, x):
        return x + 1


proxy = jep.jproxy(MyCallback(), ["jpype.benchmark.DeepBench$Callback"])


def proxy_callback():
    return DeepBench.invokeCallback(proxy, 5)


class MyObjCallback:
    def handle(self, o):
        return o


obj_proxy = jep.jproxy(MyObjCallback(), ["jpype.benchmark.DeepBench$ObjectCallback"])
callback_arg = JObject()


def proxy_object_arg():
    return DeepBench.invokeObjectCallback(obj_proxy, callback_arg)


with open(out_path, 'w') as f:
    f.write("=== jep: deep conversion paths ===\n")
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
        f.write(format_row(name, best, median) + "\n")
