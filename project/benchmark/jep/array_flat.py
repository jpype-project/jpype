"""Flat (1D) array conversion, jep side, at increasing sizes. Companion:
jpype/array_flat.py, jpy/array_flat.py -- same operations and sizes, using
the shared jpype.benchmark.DeepBench test class. See ../array_multidim.py
and ../README.md.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout.

Two categories per direction, not one "arrays" bucket -- a plain Python
list and a buffer-protocol object (numpy) hit genuinely different jep
code paths (jep_numpy.c/convert_p2j.c), confirmed from source:
  - push, "list->array": `pyfastsequence_as_jobject`'s primitive-array
    macro -- a per-element `PySequence_Fast_GET_ITEM` + convert loop.
  - push, "buffer->array": `convert_pyndarray_jprimitivearray` -- jep's
    genuine numpy fast path, a bulk `SetIntArrayRegion`, no per-element
    Python-level access at all.
  - pull, "array->list"/"array->buffer": NOT a fast-vs-slow pair here --
    see the note below. Both go through the same generic path.

pull is NOT a fast path in jep, list or buffer: a returned Java array
always comes back as jep's own `pyjarray` wrapper, which has no
buffer-protocol support at all (only jep.NDArray gets an automatic numpy
conversion on return -- confirmed: no getbufferproc in pyjarray.c). Both
`list(...)` and `np.asarray(...)` on a pyjarray go through the same
generic Python sequence protocol (__len__/__getitem__) regardless of
size -- kept as two rows anyway for a direct side-by-side with the other
two libraries' array->list/array->buffer rows, not because jep
distinguishes them itself. See ../array_multidim.py's pull numbers for
just how much this costs at scale.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_flat.py <output_path>
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
import numpy as np

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_flat_results.txt'

SIZES = [100, 1_000, 10_000, 100_000]


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


with open(out_path, 'w') as f:
    def run(name, fn, total_elements):
        n, warmup = calls_for(total_elements)
        best, median = timeit(fn, n=n, warmup=warmup)
        f.write(format_row(name, best, median) + "\n")

    f.write("=== jep: list->array, flat, push (Python -> Java) ===\n")
    for size in SIZES:
        lst = list(range(size))
        run(f"list->array int[{size}], fresh",
            lambda lst=lst: DeepBench.sumIntArray(lst), size)

    f.write("=== jep: buffer->array, flat, push (Python -> Java) ===\n")
    for size in SIZES:
        arr = np.arange(size, dtype=np.int32)
        run(f"buffer->array int[{size}], fresh",
            lambda arr=arr: DeepBench.sumIntArray(arr), size)

    f.write("=== jep: array->list, flat, pull (Java -> Python) ===\n")
    for size in SIZES:
        run(f"array->list int[{size}]",
            lambda size=size: list(DeepBench.makeIntArray(size)), size)

    f.write("=== jep: array->buffer, flat, pull (Java -> Python) ===\n")
    for size in SIZES:
        run(f"array->buffer int[{size}]",
            lambda size=size: np.asarray(DeepBench.makeIntArray(size)), size)
