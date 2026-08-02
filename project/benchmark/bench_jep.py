"""General per-call overhead benchmark, jep side -- same operations as
bench_jpype.py, for a direct comparison. See README.md.

jep embeds CPython inside a JVM process (the reverse of jpype/jpy), and
its embedded interpreter's stdout is not connected to the launching
process's stdout, so results are written to a file instead of printed.

Usage (see README.md for the exact classpath/library-path/PYTHONPATH this
needs):
    java -classpath <jep.jar> -Djava.library.path=<jep native lib dir> \
        jep.Run project/benchmark/bench_jep.py <output_path>
"""
import sys
import time

# jep's embedded execution context doesn't define __file__, so this can't
# sys.path-import _common.py the way the other two scripts do -- inlined
# instead. Keep in sync with _common.py by hand if it changes.


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


from java.lang import Math, Integer, String

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_results.txt'

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


def string_roundtrip():
    s = String("hello")
    return str(s)


with open(out_path, 'w') as f:
    f.write("=== jep ===\n")
    for name, fn in (
            ("Math.max(int,int)", math_max),
            ("new Integer(int)", box_integer),
            ("new String + toString", string_roundtrip),
    ):
        best, median = timeit(fn)
        f.write(format_row(name, best, median) + "\n")
