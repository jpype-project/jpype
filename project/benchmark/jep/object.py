""""object" category, jep side: plain Object identity (argument + return
value). Companion: jpype/object.py, jpy/object.py -- same operation, using
the shared jpype.benchmark.DeepBench test class. See ../README.md.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/object.py <output_path>
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

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_object_results.txt'

obj = JObject()


def object_identity():
    return DeepBench.identity(obj)


with open(out_path, 'w') as f:
    f.write("=== jep: object ===\n")
    best, median = timeit(object_identity)
    f.write(format_row("Object identity", best, median) + "\n")
