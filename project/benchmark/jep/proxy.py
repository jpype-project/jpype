""""proxy" category, jep side: Java calling back into Python through an
established callback binding (jep.jproxy(pyobj, [interfaces]), also
constructed once) -- both a primitive `int` argument and an `Object`
argument. Companion: jpype/proxy.py -- same operations, using the shared
jpype.benchmark.DeepBench test class. No jpy/proxy.py: see ../README.md
for why jpy's proxy mechanism didn't work from Python in this checkout.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/proxy.py <output_path>
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

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_proxy_results.txt'


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
    f.write("=== jep: proxy ===\n")
    for name, fn in (
            ("proxy callback (established), int arg", proxy_callback),
            ("proxy callback (established), Object arg", proxy_object_arg),
    ):
        best, median = timeit(fn)
        f.write(format_row(name, best, median) + "\n")
