""""method dispatch" category, jep side: overload resolution across 16
candidates, monomorphic and polymorphic call sites. Companion:
jpype/dispatch.py, jpy/dispatch.py -- same operations, using the shared
jpype.benchmark.DeepBench test class. See ../README.md.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/dispatch.py <output_path>
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

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_dispatch_results.txt'

t0_obj = DeepBench.T0()
t15_obj = DeepBench.T15()

state = {'flip': False}


def overload_monomorphic():
    return DeepBench.call(t15_obj)


def overload_polymorphic():
    state['flip'] = not state['flip']
    return DeepBench.call(t0_obj if state['flip'] else t15_obj)


with open(out_path, 'w') as f:
    f.write("=== jep: method dispatch ===\n")
    for name, fn in (
            ("overload x16, monomorphic", overload_monomorphic),
            ("overload x16, polymorphic", overload_polymorphic),
    ):
        best, median = timeit(fn)
        f.write(format_row(name, best, median) + "\n")
