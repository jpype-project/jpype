""""int" category, jep side: Math.max(int,int) and new Integer(int).
Companion: jpype/int.py, jpy/int.py -- same two operations. See
../README.md.

jep's embedded execution context doesn't define __file__, so this can't
sys.path-import _common.py the way the other two scripts do -- inlined
instead. Keep in sync with _common.py by hand if it changes. Results are
written to a file since jep's embedded stdout isn't connected to the
launching process's stdout.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH):
    java -classpath <jep.jar> -Djava.library.path=<jep native lib dir> \
        jep.Run project/benchmark/jep/int.py <output_path>
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


from java.lang import Math, Integer

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_int_results.txt'

i = 0


def math_max():
    global i
    i += 1
    return Math.max(i, i + 1)


def box_integer():
    global i
    i += 1
    return Integer(i)


with open(out_path, 'w') as f:
    f.write("=== jep: int ===\n")
    for name, fn in (
            ("Math.max(int,int)", math_max),
            ("new Integer(int)", box_integer),
    ):
        best, median = timeit(fn)
        f.write(format_row(name, best, median) + "\n")
