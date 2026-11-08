# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
"""Regressions for races found while reviewing the free-threaded support.

Every scenario that touches process-wide state (JVM start, registered
conversions, patched hooks) runs in its own interpreter, so these tests are
safe under the parallel runner.
"""
import os
from pathlib import Path
import subprocess
import sys
import threading
import unittest

import common

CLASSES = Path(__file__).resolve().parents[1] / "classes"


def _run(script, marker, timeout=60):
    env = dict(os.environ)
    # The scenarios control the classpath themselves.
    env.pop("CLASSPATH", None)
    result = subprocess.run(
        [sys.executable, "-c", script, str(CLASSES)],
        capture_output=True, text=True, timeout=timeout, env=env,
    )
    output = result.stdout + result.stderr
    if result.returncode != 0 or marker not in result.stdout:
        raise AssertionError("scenario failed (rc=%d):\n%s" % (result.returncode, output))


class ClasspathStartupRaceTest(unittest.TestCase):

    def testAddClassPathDuringStartIsNotLost(self):
        """addClassPath() racing startJVM() must still reach the class loader.

        startJVM snapshots the classpath long before the JVM runs.  A path
        added in between used to be recorded only in the Python list (the
        JVM was "not started yet") and never loaded.
        """
        script = r'''
import sys
import threading
import time
import _jpype
import jpype

added = threading.Event()
original = _jpype.startup

def add():
    jpype.addClassPath(sys.argv[1])
    added.set()

adder = threading.Thread(target=add, daemon=True)

def slow_startup(*args, **kwargs):
    # The classpath has been snapshotted; the JVM is not running yet.  Race
    # an addClassPath() into exactly this window.  Without the fix it
    # completes here (and is lost); with the fix it waits for startJVM.
    adder.start()
    time.sleep(0.2)
    return original(*args, **kwargs)

_jpype.startup = slow_startup
# Start on the main thread: a JVM started from a worker thread hangs at exit.
jpype.startJVM()
adder.join(30)
assert not adder.is_alive(), "addClassPath deadlock"
assert added.is_set()
jpype.JClass("jpype.classhints.ClassHintsTest")  # ClassNotFound without the fix
print("CLASSPATH OK", flush=True)
'''
        _run(script, "CLASSPATH OK")


class DocBuildDeadlockTest(unittest.TestCase):

    def testDocReadFromCustomizerDuringConcurrentDocBuild(self):
        """A customizer reading __doc__ must not deadlock against a doc build.

        Thread A builds ArrayList.__doc__ and, while doing so, needs a class
        wrapper (type factory monitor).  Thread B holds that monitor while
        running a customizer that reads ArrayList.__doc__.  If B waits for A's
        build to finish, neither thread can make progress.
        """
        script = r'''
import os
import sys
import threading
import _jpype
import jpype

X = "java.util.ArrayList"
Y = "jpype.overloads.DerivedTest"
a_building = threading.Event()
b_in_post = threading.Event()
errors = []
original_doc = _jpype._jclassDoc

def doc(cls):
    if cls.__name__ == X and threading.current_thread().name == "A":
        if not b_in_post.wait(10):
            raise RuntimeError("B never entered the customizer")
        a_building.set()
        jpype.JClass(Y)   # blocks on the type factory monitor held by B
    return original_doc(cls)

_jpype._jclassDoc = doc
jpype.startJVM(classpath=[sys.argv[1]])
ArrayList = jpype.JClass(X)

@jpype.JImplementationFor(Y)
class _Customizer:
    def __jclass_init__(cls):
        if threading.current_thread().name == "B":
            b_in_post.set()
            if not a_building.wait(10):
                raise RuntimeError("A never started building the doc")
            ArrayList.__doc__

def run(fn):
    def target():
        try:
            fn()
        except BaseException as ex:
            errors.append(ex)
    return target

a = threading.Thread(target=run(lambda: ArrayList.__doc__), name="A", daemon=True)
b = threading.Thread(target=run(lambda: jpype.JClass(Y)), name="B", daemon=True)
b.start()
a.start()
a.join(20)
b.join(20)
if a.is_alive() or b.is_alive():
    print("DEADLOCK", flush=True)
    os._exit(1)   # threads are stuck in native code; do not wait for them
assert not errors, errors
assert isinstance(ArrayList.__doc__, str)
print("NO DEADLOCK", flush=True)
'''
        _run(script, "NO DEADLOCK")


class ConversionRegistrationRaceTest(unittest.TestCase):

    def testRegisterConversionsWhileResolving(self):
        """Registering a JConversion while other threads resolve overloads.

        The per-class conversion list was a plain std::list that was appended
        to while method calls iterated it.
        """
        script = r'''
import sys
import threading
import jpype

jpype.startJVM(classpath=[sys.argv[1]])
name = "jpype.classhints.Custom"
harness = jpype.JClass("jpype.classhints.ClassHintsTest")

def to_custom(cls, value):
    return jpype.JObject(None, cls)

class Seed:
    pass

jpype.JConversion(name, exact=Seed)(to_custom)
writers_done = threading.Event()
registered = []
errors = []
barrier = threading.Barrier(8, timeout=10)

def writer(n):
    try:
        barrier.wait()
        for i in range(500):
            tp = type("T%d_%d" % (n, i), (), {})
            jpype.JConversion(name, exact=tp)(to_custom)
            registered.append(tp)
    except BaseException as ex:
        errors.append(ex)

def reader():
    try:
        barrier.wait()
        while not writers_done.is_set():
            harness.call(Seed())
    except BaseException as ex:
        errors.append(ex)

writers = [threading.Thread(target=writer, args=(n,)) for n in range(4)]
readers = [threading.Thread(target=reader) for _ in range(4)]
for t in writers + readers:
    t.start()
for t in writers:
    t.join(60)
writers_done.set()
for t in readers:
    t.join(60)
assert all(not t.is_alive() for t in writers + readers), "hang"
assert not errors, errors
assert len(registered) == 2000
for tp in registered:
    harness.call(tp())   # every registration must have been kept
print("CONVERSIONS OK", flush=True)
'''
        _run(script, "CONVERSIONS OK", timeout=120)


class HintsVersionTest(common.JPypeTestCase):

    def testConcurrentVersionBumpsAreNotLost(self):
        """Every registration bumps the hints version exactly once.

        The cached conversion info is rebuilt only when the version changes,
        so a lost bump can hide a registered conversion from ``_hints``.
        """
        from jpype._jcustomizer import JClassHints

        hints = JClassHints()  # private instance, not registered for any class
        threads_count, per_thread = 8, 500
        barrier = threading.Barrier(threads_count, timeout=10)
        start = hints._version

        def work():
            barrier.wait()
            for _ in range(per_thread):
                hints._excludeConversion(int)

        threads = [threading.Thread(target=work) for _ in range(threads_count)]
        for t in threads:
            t.start()
        for t in threads:
            t.join(30)
        self.assertTrue(all(not t.is_alive() for t in threads))
        self.assertEqual(hints._version - start, threads_count * per_thread)


@unittest.skipUnless(sys.platform.startswith("linux"), "reads /proc/self/status")
class ArrayExportRaceTest(unittest.TestCase):

    def testLosingViewReleasesJavaElements(self):
        """Concurrent first exports of a primitive array must not leak.

        Every exporting thread builds a view, which pins or copies the Java
        elements, and only one of them is published.  The losing views used
        to be deleted without releasing the elements, which leaks a native
        copy of the whole array per lost race on HotSpot.
        """
        script = r'''
import threading
import jpype

jpype.startJVM()

def rss_mb():
    with open("/proc/self/status") as f:
        for line in f:
            if line.startswith("VmRSS"):
                return int(line.split()[1]) // 1024

SIZE = 1000000        # 4 MB per copy
threads_count = 8
barrier = threading.Barrier(threads_count, timeout=30)
array = jpype.JArray(jpype.JInt)(SIZE)
errors = []

def export(rounds):
    for _ in range(rounds):
        barrier.wait()
        with memoryview(array) as view:
            assert view.nbytes == 4 * SIZE
        barrier.wait()  # all released: the next round races a new export

def run(rounds):
    def work():
        try:
            export(rounds)
        except BaseException as ex:
            errors.append(ex)
            barrier.abort()
    threads = [threading.Thread(target=work) for _ in range(threads_count)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(120)
    assert all(not t.is_alive() for t in threads), "hang"
    assert not errors, errors

run(20)               # warm up
start = rss_mb()
run(300)
grown = rss_mb() - start
# With the leak this grew by several hundred MB on a free-threaded build.
assert grown < 150, "RSS grew by %d MB" % grown
print("ARRAY EXPORT OK", flush=True)
'''
        _run(script, "ARRAY EXPORT OK", timeout=180)


class SlotTraverseRaceTest(unittest.TestCase):

    def testTraverseWhileSlotsChange(self):
        """tp_traverse reads the lazy slots without a lock.

        This is only safe because every caller of tp_traverse stops the world
        on free-threaded builds.  Replace __doc__/__annotations__ on shared
        method objects while other threads collect and call
        gc.get_referents(), which would visit freed objects otherwise.
        """
        script = r'''
import gc
import threading
import jpype

jpype.startJVM()
s = jpype.JClass("java.lang.String")("x")
methods = [getattr(s, n) for n in ("length", "trim", "isEmpty", "hashCode")] * 8
stop = threading.Event()
errors = []

def writer(i):
    try:
        n = 0
        while not stop.is_set():
            m = methods[(i + n) % len(methods)]
            m.__doc__ = "doc %d %d" % (i, n)   # a fresh object every time
            m.__annotations__ = {"return": str(n)}
            m.__doc__
            n += 1
    except BaseException as ex:
        errors.append(ex)

def collector():
    try:
        while not stop.is_set():
            gc.collect()
            for m in methods:
                gc.get_referents(m)
    except BaseException as ex:
        errors.append(ex)

threads = [threading.Thread(target=writer, args=(i,)) for i in range(4)]
threads += [threading.Thread(target=collector) for _ in range(2)]
for t in threads:
    t.start()
stop.wait(3)
stop.set()
for t in threads:
    t.join(30)
assert all(not t.is_alive() for t in threads), "hang"
assert not errors, errors
print("TRAVERSE OK", flush=True)
'''
        _run(script, "TRAVERSE OK", timeout=60)


class ClasspathCustomizerDeadlockTest(unittest.TestCase):

    def testAddClassPathFromCustomizerWhileAnotherThreadAdds(self):
        """addClassPath() must not call Java while holding the classpath lock.

        Loading a path into the running JVM creates class wrappers, which
        takes the type factory monitor.  A customizer runs with that monitor
        held; if it calls addClassPath() while another thread holds the
        classpath lock and waits for the monitor, neither can proceed.
        """
        script = r'''
import sys
import tempfile
import threading
import time
import jpype

in_customizer = threading.Event()
first, second = tempfile.mkdtemp(), tempfile.mkdtemp()

@jpype.JImplementationFor("jpype.classhints.Custom")
class _Customizer:
    def __jclass_init__(cls):
        # Runs with the type factory monitor held.
        in_customizer.set()
        time.sleep(0.5)               # let the other thread enter addClassPath
        jpype.addClassPath(second)

jpype.startJVM(classpath=[sys.argv[1]])
creator = threading.Thread(target=lambda: jpype.JClass("jpype.classhints.Custom"), daemon=True)
creator.start()
assert in_customizer.wait(10), "customizer did not run"
adder = threading.Thread(target=lambda: jpype.addClassPath(first), daemon=True)
adder.start()
creator.join(15)
adder.join(15)
assert not creator.is_alive() and not adder.is_alive(), "classpath deadlock"
paths = jpype.getClassPath(env=False)
assert first in paths and second in paths, paths
print("CLASSPATH OK", flush=True)
'''
        _run(script, "CLASSPATH OK", timeout=60)
