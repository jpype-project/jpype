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


with open(out_path, 'w') as f:
    f.write("=== jep: deep conversion paths ===\n")
    for name, fn in (
            ("overload x16, monomorphic", overload_monomorphic),
            ("overload x16, polymorphic", overload_polymorphic),
            ("int[] from list(100), fresh", array_arg),
            ("Object identity", object_identity),
            ("proxy callback (established)", proxy_callback),
    ):
        best, median = timeit(fn)
        f.write(format_row(name, best, median) + "\n")
