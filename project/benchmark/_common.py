"""Shared timing helper for the cross-library benchmark scripts in this
directory (bench_jpype.py, bench_jpy.py, bench_jep.py).

Kept dependency-free (stdlib only) since bench_jep.py runs inside jep's
embedded CPython, not a normal venv.
"""
import csv
import time


def timeit(fn, n=200_000, warmup=1000, trials=7):
    """Returns (best_ns_per_call, median_ns_per_call)."""
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
    best = samples[0]
    median = samples[len(samples) // 2]
    return best, median


def format_row(name, best, median):
    return f"{name:32s} best={best:8.1f} ns/call  median={median:8.1f} ns/call"


class CsvLog:
    """Appends one row per benchmark call to a CSV file alongside the
    human-readable printed output, so a report can be built by reading
    exact recorded numbers back out instead of transcribing printed
    tables by hand."""

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
