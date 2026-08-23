"""Flat (1D) array conversion, jep side, at increasing sizes. Swept across
four primitive element types (int32, int64, float32, float64), matching
jpype/array_flat.py's type sweep -- a per-type gap, not just a per-size
one, should show up here too if there is one. Companion: jpype/array_flat.py,
jpy/array_flat.py -- same operations and sizes, using the shared
jpype.benchmark.DeepBench test class. See ../array_multidim.py and
../README.md.

See jep/int.py for why this inlines timeit/format_row instead of
importing _common.py, and writes results to a file instead of stdout. For
the same reason, _common.CsvLog can't be imported either -- the CSV
writer below is an inlined equivalent (plain csv.DictWriter over a file
opened with newline='', header written once, one writerow()+flush() per
sample), same shape as _common.CsvLog, just copied in by hand.

Two categories per direction, not one "arrays" bucket -- a plain Python
list and a buffer-protocol object (numpy) hit genuinely different jep
code paths (jep_numpy.c/convert_p2j.c), confirmed from source:
  - push, "list->array": `pyfastsequence_as_jobject`'s primitive-array
    macro -- a per-element `PySequence_Fast_GET_ITEM` + convert loop.
  - push, "buffer->array": `convert_pyndarray_jprimitivearray` -- jep's
    genuine numpy fast path, a bulk `Set<Type>ArrayRegion`, no
    per-element Python-level access at all.
  - pull, "array->list"/"array->buffer": NOT a fast-vs-slow pair here --
    see the note below. Both go through the same generic path.

pull is NOT a fast path in jep, list or buffer, for any of the four
types: a returned Java array always comes back as jep's own `pyjarray`
wrapper, which has no buffer-protocol support at all (only jep.NDArray
gets an automatic numpy conversion on return -- confirmed: no
getbufferproc in pyjarray.c). Both `list(...)` and `np.asarray(...)` on a
pyjarray go through the same generic Python sequence protocol
(__len__/__getitem__) regardless of size or element type -- kept as two
rows anyway for a direct side-by-side with the other two libraries'
array->list/array->buffer rows, not because jep distinguishes them
itself. jep has no toList()-equivalent fast pull path at all, so unlike
jpype's array_flat.py there is no fifth "array->list via toList()" row
here. See ../array_multidim.py's pull numbers for just how much the
generic path costs at scale.

Writes a CSV (fieldnames: category, direction, source, dtype, size, n,
best_ns, median_ns) to the path given as argv[2], defaulting to
"array_flat_results.csv" next to out_path.

Usage (see ../README.md for the exact classpath/library-path/PYTHONPATH,
which for this one also needs test/classes + test/harness on top of
jep.jar):
    java -classpath <jep.jar>:<test/classes>:<test/harness> \
        -Djava.library.path=<jep native lib dir> jep.Run \
        project/benchmark/jep/array_flat.py <output_path> [<csv_path>]
"""
import sys
import os
import csv
import time


def timeit(fn, n=200_000, warmup=1000, trials=7):
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


class CsvLog:
    """Inlined equivalent of _common.CsvLog -- jep's embedded interpreter
    can't sys.path-import project/benchmark/_common.py (no __file__), so
    this is copied in by hand rather than shared."""

    def __init__(self, path, fieldnames):
        self._fieldnames = fieldnames
        self._f = open(path, 'w', newline='')
        self._writer = csv.DictWriter(self._f, fieldnames=fieldnames)
        self._writer.writeheader()

    def write(self, **row):
        self._writer.writerow(row)
        self._f.flush()

    def close(self):
        self._f.close()


from jpype.benchmark import DeepBench
import numpy as np

out_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bench_jep_array_flat_results.txt'
csv_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(
    os.path.dirname(out_path) or '.', 'array_flat_results.csv')

SIZES = [100, 1_000, 10_000, 100_000]

# (label, numpy dtype, sum{Type}Array, make{Type}Array)
TYPES = [
    ('int', np.dtype('int32'), DeepBench.voidIntArray, DeepBench.makeIntArray),
    ('long', np.dtype('int64'), DeepBench.voidLongArray, DeepBench.makeLongArray),
    ('float', np.dtype('float32'), DeepBench.voidFloatArray, DeepBench.makeFloatArray),
    ('double', np.dtype('float64'), DeepBench.voidDoubleArray, DeepBench.makeDoubleArray),
]

csv_log = CsvLog(
    csv_path,
    ['category', 'direction', 'source', 'dtype', 'size', 'n', 'best_ns', 'median_ns'])


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


with open(out_path, 'w') as f:
    def run(name, fn, total_elements, direction, source, dtype):
        n, warmup = calls_for(total_elements)
        best, median = timeit(fn, n=n, warmup=warmup)
        f.write(format_row(name, best, median) + "\n")
        csv_log.write(category='array_flat', direction=direction, source=source,
                      dtype=dtype, size=total_elements, n=n, best_ns=best, median_ns=median)

    for label, dtype, sumfn, makefn in TYPES:
        f.write(f"=== jep: list->array, flat, push (Python -> Java), {label} ===\n")
        for size in SIZES:
            # Python-level element type matches the target array's own kind
            # -- exercises each type's own homogeneous push, not a widening
            # conversion from a different Python type.
            lst = [float(i) for i in range(size)] if dtype.kind == 'f' else list(range(size))
            run(f"list->array {label}[{size}], fresh",
                lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
                'push', 'list', label)

        if dtype.kind == 'f':
            f.write(f"=== jep: list->array, flat, push (Python -> Java), {label}, widening from int ===\n")
            for size in SIZES:
                # A plain Python int list pushed into a float[]/double[]
                # target -- idiomatic, and not the same benchmark as the
                # homogeneous-type row above.
                lst = list(range(size))
                run(f"list->array {label}[{size}], widening from int",
                    lambda lst=lst, sumfn=sumfn: sumfn(lst), size,
                    'push', 'list_widen', label)

        f.write(f"=== jep: buffer->array, flat, push (Python -> Java), {label} ===\n")
        for size in SIZES:
            arr = np.arange(size, dtype=dtype)
            run(f"buffer->array {label}[{size}], fresh",
                lambda arr=arr, sumfn=sumfn: sumfn(arr), size,
                'push', 'buffer', label)

        f.write(f"=== jep: array->list, flat, pull (Java -> Python), {label} ===\n")
        for size in SIZES:
            run(f"array->list {label}[{size}]",
                lambda size=size, makefn=makefn: list(makefn(size)), size,
                'pull', 'list', label)

        f.write(f"=== jep: array->buffer, flat, pull (Java -> Python), {label} ===\n")
        for size in SIZES:
            run(f"array->buffer {label}[{size}]",
                lambda size=size, makefn=makefn: np.asarray(makefn(size)), size,
                'pull', 'buffer', label)

csv_log.close()
