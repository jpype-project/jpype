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


class ConversionInfoRaceTest(unittest.TestCase):

    def testConversionInfoIsNeverOlderThanTheCall(self):
        """``_conversionInfo()`` lists every conversion registered before it.

        The conversion lists used to be cached as attributes on the shared
        hints object, guarded by a version counter.  Concurrent builders could
        overwrite newer lists with older ones, so a reader got lists missing
        conversions registered before its call.  The lists are now built per
        call and shared with nobody.
        """
        script = r'''
import sys
import threading
import jpype

jpype.startJVM(classpath=[sys.argv[1]])
name = "jpype.classhints.Custom"
Custom = jpype.JClass(name)

def to_custom(cls, value):
    return jpype.JObject(None, cls)

writers_count, readers_count, per_writer = 4, 8, 300
registered = []
stale = []
errors = []
done = threading.Event()
barrier = threading.Barrier(writers_count + readers_count, timeout=30)

def writer(n):
    try:
        barrier.wait()
        for i in range(per_writer):
            tp = type("T%d_%d" % (n, i), (), {})
            jpype.JConversion(name, exact=tp)(to_custom)
            registered.append(tp)   # only after the registration returned
    except BaseException as ex:
        errors.append(ex)

def reader():
    try:
        barrier.wait()
        while not done.is_set():
            before = registered[:len(registered)]
            implicit = Custom._conversionInfo().implicit
            if before and before[-1] not in implicit:
                stale.append(len(before))
    except BaseException as ex:
        errors.append(ex)

writers = [threading.Thread(target=writer, args=(n,)) for n in range(writers_count)]
readers = [threading.Thread(target=reader) for _ in range(readers_count)]
for t in writers + readers:
    t.start()
for t in writers:
    t.join(60)
done.set()
for t in readers:
    t.join(60)
assert all(not t.is_alive() for t in writers + readers), "hang"
assert not errors, errors
assert not stale, "%d reads returned conversion info older than the call" % len(stale)
implicit = Custom._conversionInfo().implicit
assert all(tp in implicit for tp in registered), "final info is missing conversions"
print("CONVERSION INFO OK", flush=True)
'''
        _run(script, "CONVERSION INFO OK", timeout=120)
