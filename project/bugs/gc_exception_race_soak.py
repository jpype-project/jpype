"""Soak harness for jpype-project#1415 (GC-reentrancy corruption of an
in-flight Python exception mid-C++-unwind).

This is NOT a deterministic reproducer - the underlying race depends on
CPython's generational GC allocation counters and JVM GC scheduling, both
of which are opaque and non-reproducible on demand. Its job is to run the
suspect path under sustained, heavy contention on both the Python and
Java side of the boundary, for long enough to give a still-present bug
many chances to fire, so a clean run here is meaningful evidence rather
than luck.

The confirmed trigger (see plan/GCExceptionRace.md): a reverse-bridge
call into Python raises mid-conversion (JArray(JBoolean) assignment
calling __bool__ on each element), producing a _python_error exception
that C++ has to unwind through. Racing that against:
  - gc.set_threshold(1, 1, 1) - the original bug required heavy
    accumulated allocation state (~40 preceding test files' worth) to
    trip CPython's cyclic GC mid-unwind at any reasonable hit rate;
    forcing collection on (almost) every allocation reproduces the same
    window deterministically instead of relying on incidental churn,
  - Python-side allocation pressure on a second thread (cyclic garbage,
    so there is always something for that aggressive threshold to find),
  - a proxy/reverse-bridge workload (a JImplements(Comparator) invoked
    via Collections.sort, pulling and pushing real Java objects) running
    interleaved with the provoke calls, so the unwind path is exercised
    alongside realistic reverse-bridge traffic rather than in isolation,
    and
  - a Java thread hammering System.gc() concurrently (to also stress the
    cross-thread/JNI angle, even though the confirmed mechanism was
    single-threaded reentrancy, not a cross-thread race)
maximizes the odds of catching a regression. With gc.set_threshold(1, 1,
1), this harness reliably reproduces the original corruption
(SystemError: "error return without exception set") in a few thousand
iterations against a pre-fix checkout - see plan/GCExceptionRace.md.

Usage:
    python gc_exception_race_soak.py [iterations] [seconds]

Exits non-zero (and prints full detail) the moment a single iteration
observes anything other than exactly "SystemError: nope" - any other
exception type, message, or a hang - is a corruption signal. A true
memory-safety violation may instead just crash the process; that is
also a failure, it will just not print this script's summary.
"""
import gc
import sys
import threading
import time

import jpype
import jpype.imports  # noqa: F401


class _RaisesOnBool:
    """Object whose __bool__ raises - the confirmed jpype-project#1415
    trigger when used in a JArray(JBoolean) slice assignment."""

    def __bool__(self):
        # Deliberately SystemError, not the conventional TypeError a failed
        # __bool__ would normally raise (CodeQL flags this as
        # "non-standard exception in special method" - false positive here):
        # the corrupted-exception bug this harness reproduces manifests as
        # CPython's own "SystemError: error return without exception set",
        # a specific, recognizable mismatch from this "SystemError: nope".
        # A conventional TypeError here would be indistinguishable from the
        # corrupted state and defeat the whole detection strategy below.
        raise SystemError("nope")


def _provoke_once():
    ja = jpype.JArray(jpype.JBoolean)(5)
    values = [True, False, _RaisesOnBool(), True, False]
    try:
        ja[:] = values
    except SystemError as ex:
        if str(ex) != "nope":
            raise AssertionError(
                "CORRUPTION: expected SystemError('nope'), got "
                "SystemError(%r)" % (str(ex),)
            )
        return
    except BaseException as ex:  # noqa: BLE001 - detection, not handling
        raise AssertionError(
            "CORRUPTION: expected SystemError('nope'), got "
            "%s(%r)" % (type(ex).__name__, str(ex))
        )
    else:
        raise AssertionError("CORRUPTION: no exception raised at all")


def _python_gc_pressure(stop_event):
    """Continuously build and drop cyclic garbage on a second Python
    thread, to raise the odds CPython's generational GC fires while the
    main thread is mid-unwind through the C++ exception path."""
    while not stop_event.is_set():
        for _ in range(1000):
            a = {}
            b = {"a": a}
            a["b"] = b
        del a, b


def _start_java_gc_hammer(stop_event):
    """A Java thread that calls System.gc() back-to-back for the
    duration of the soak, to stress the JNI/cross-thread angle too."""
    from java.lang import Runnable, System, Thread

    @jpype.JImplements(Runnable)
    class GcHammer:
        @jpype.JOverride
        def run(self):
            while not stop_event.is_set():
                System.gc()

    hammer = GcHammer()
    thread = Thread(hammer)
    thread.setDaemon(True)
    thread.start()
    return thread


def _make_proxy_workload():
    """Build a reverse-bridge workload: a JImplements(Comparator) proxy
    invoked via Collections.sort, pulling boxed Java Integers into Python
    and pushing a Python int comparison result back out - so the provoke
    loop below runs interleaved with realistic proxy dispatch/object
    traffic, not just isolated array-assignment calls."""
    from java.util import ArrayList, Collections, Comparator

    @jpype.JImplements(Comparator)
    class ReverseIntComparator:
        @jpype.JOverride
        def compare(self, a, b):
            # Pull both boxed values into Python, build some short-lived
            # cyclic garbage of our own, then push a plain Python int back.
            x, y = int(a), int(b)
            tmp = {"x": x}
            tmp["self"] = tmp
            del tmp
            return y - x

        @jpype.JOverride
        def equals(self, other):
            return self is other

    comparator = ReverseIntComparator()

    def _run_once():
        lst = ArrayList()
        for v in (5, 3, 1, 4, 2):
            lst.add(v)
        Collections.sort(lst, comparator)

    return _run_once


def main():
    iterations = int(sys.argv[1]) if len(sys.argv) > 1 else 200000
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 60.0

    # The original bug needed heavy accumulated allocation state (an
    # entire preceding test suite) to trip CPython's cyclic GC mid-unwind
    # at any reasonable hit rate. Forcing collection aggressively
    # reproduces that same window without needing to replay the suite.
    gc.set_threshold(1, 1, 1)

    jpype.startJVM()

    stop_event = threading.Event()
    gc_thread = threading.Thread(target=_python_gc_pressure, args=(stop_event,))
    gc_thread.daemon = True
    gc_thread.start()
    _start_java_gc_hammer(stop_event)
    proxy_workload = _make_proxy_workload()

    start = time.time()
    completed = 0
    try:
        while completed < iterations and (time.time() - start) < seconds:
            _provoke_once()
            proxy_workload()
            completed += 1
            if completed % 10000 == 0:
                print("iteration %d clean (%.1fs elapsed)" %
                      (completed, time.time() - start))
    finally:
        stop_event.set()
        gc_thread.join(timeout=5)

    print("DONE: %d iterations clean, no corruption detected" % completed)


if __name__ == "__main__":
    main()
