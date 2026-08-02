""""string" category, jep side: new String(...) + .toString(). Companion:
jpype/strings.py, jpy/strings.py -- same operation. See ../README.md.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH):
    java -classpath <jep.jar> -Djava.library.path=<jep native lib dir> \
        jep.Run project/benchmark/jep/strings.py <output_path>
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


from java.lang import String

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_string_results.txt'


def string_roundtrip():
    s = String("hello")
    return str(s)


with open(out_path, 'w') as f:
    f.write("=== jep: string ===\n")
    best, median = timeit(string_roundtrip)
    f.write(format_row("new String + toString", best, median) + "\n")
