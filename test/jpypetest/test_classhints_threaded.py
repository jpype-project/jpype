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
"""Regressions for concurrent first use of class conversion hints (#1507).

The subprocess tests are deliberately not marked ``thread_unsafe``: each runs
its interleaving in its own process, so the parallel runner may call them
concurrently.  Tests that patch process-wide state are marked individually.
"""
from pathlib import Path
import subprocess
import sys
import unittest
import common

class ClassHintsThreadedTest(unittest.TestCase):
    def testConversionDuringClassInitialization(self):
        script = r'''
import sys
import threading
import _jpype
import jpype

published = threading.Event()
release = threading.Event()
started = threading.Event()
finished = threading.Event()
ready = threading.Event()
converted = threading.Event()
errors = []
original_post = _jpype._jclassPost
name = "jpype.classhints.Custom"

def delayed_post(cls, *args):
    if cls.__name__ == name and sys.argv[2] not in ("late", "late-fail"):
        published.set()
        if not release.wait(5):
            raise RuntimeError("class initialization was not released")
        if sys.argv[2] in ("fail", "fail-java"):
            raise RuntimeError("intentional class initialization failure")
    return original_post(cls, *args)

_jpype._jclassPost = delayed_post
jpype.startJVM(classpath=[sys.argv[1]])
harness = jpype.JClass("jpype.classhints.ClassHintsTest")
value = jpype.JString("value") if sys.argv[2] == "fail-java" else "value"

if sys.argv[2] in ("late", "late-fail"):
    @jpype.JImplementationFor(name)
    class LateCustomizer:
        def __jclass_init__(cls):
            published.set()
            if not release.wait(5):
                raise RuntimeError("class customizer was not released")
            if sys.argv[2] == "late-fail":
                raise RuntimeError("intentional class customizer failure")
            ready.set()

@jpype.JConversion(name, exact=str)
def convert(cls, value):
    converted.set()
    if sys.argv[2] == "late":
        assert ready.is_set(), "conversion used an unfinished class customizer"
    return jpype.JObject(None, cls)

def create():
    try:
        jpype.JClass(name)
    except BaseException as ex:
        errors.append(ex)

def call():
    started.set()
    try:
        harness.call(value)
        assert harness.input is None
    except BaseException as ex:
        errors.append(ex)
    finally:
        finished.set()

creator = threading.Thread(target=create, daemon=True)
caller = threading.Thread(target=call, daemon=True)
creator.start()
try:
    assert published.wait(5), "class wrapper was not published"
    caller.start()
    assert started.wait(5), "conversion did not start"
    assert not finished.wait(0.1), "conversion used an unfinished class"
finally:
    release.set()
creator.join(5)
caller.join(5)
assert not creator.is_alive() and not caller.is_alive(), "initialization deadlock"
if sys.argv[2] in ("fail", "fail-java", "late-fail"):
    assert len(errors) == 2, errors
    # One error is the customizer failure itself.  The other is the
    # concurrent call: it waited for the class, saw that it failed and found
    # no usable conversion, instead of using the half-built class.
    assert sum(isinstance(ex, RuntimeError) and "intentional" in str(ex)
               for ex in errors) == 1, errors
    assert sum(isinstance(ex, TypeError) for ex in errors) == 1, errors
    assert not converted.is_set(), "conversion used a failed class"
    # A failed class only drops out as a candidate; the other overloads
    # still resolve.  pick(Object) is never an exact match, so pick(Custom)
    # is always tested as a candidate.
    assert harness.pick("value") == "Object"
    assert harness.pick(jpype.JString("value")) == "Object"
    assert harness.pick(1) == "Object"
    # The failure is permanent: a later lookup must raise, not return the
    # half-built wrapper.
    try:
        jpype.JClass(name)
    except RuntimeError as ex:
        assert "failed to initialize" in str(ex), ex
    else:
        raise AssertionError("failed class was handed out again")
    print("CONVERSION FAIL SAFE", flush=True)
else:
    assert not errors, errors
    print("CONVERSION OK", flush=True)
'''
        classes = Path(__file__).resolve().parents[1] / "classes"
        for mode, marker in (("ok", "CONVERSION OK"),
                             ("fail", "CONVERSION FAIL SAFE"),
                             ("fail-java", "CONVERSION FAIL SAFE"),
                             ("late-fail", "CONVERSION FAIL SAFE"),
                             ("late", "CONVERSION OK")):
            with self.subTest(mode=mode):
                result = subprocess.run(
                    [sys.executable, "-c", script, str(classes), mode],
                    capture_output=True, text=True, timeout=30,
                )
                self.assertEqual(result.returncode, 0,
                                 result.stdout + result.stderr)
                self.assertIn(marker, result.stdout)

    @common.threadUnsafe
    def testConcurrentHintsRegistrationUsesOneObject(self):
        import threading
        from unittest.mock import patch
        import _jpype
        from jpype import _jcustomizer

        name = "jpype.classhints.ConcurrentRegistration"
        barrier = threading.Barrier(8, timeout=5)
        original = _jcustomizer.JClassHints
        hints = []
        errors = []

        def make_hints():
            barrier.wait()
            return original()

        def get_hints():
            try:
                hints.append(_jcustomizer.getClassHints(name))
            except BaseException as ex:
                errors.append(ex)

        try:
            with patch.object(_jcustomizer, "JClassHints", make_hints):
                threads = [threading.Thread(target=get_hints) for _ in range(8)]
                for thread in threads:
                    thread.start()
                for thread in threads:
                    thread.join(10)
                self.assertTrue(all(not thread.is_alive() for thread in threads))
            self.assertEqual(errors, [])
            self.assertEqual(len(hints), 8)
            self.assertTrue(all(hint is hints[0] for hint in hints))
        finally:
            _jpype._hints.pop(name, None)

    def testPrimitiveAndRootTypesAreComplete(self):
        """Types bound through class_/_hints must not be incomplete forever."""
        from jpype.types import (JBoolean, JByte, JChar, JDouble, JFloat,
                                 JInt, JLong, JObject, JShort, JString)
        # No subTest: pytest-run-parallel runs this body on several threads
        # sharing one TestCase instance, and unittest's subTest bookkeeping
        # (self._subtest) is not thread-safe.
        for cls in (JBoolean, JByte, JChar, JShort, JInt, JLong, JFloat,
                    JDouble, JString, JObject):
            self.assertTrue(cls._isComplete(), cls)
            hints = cls._hints
            self.assertIsNotNone(hints, cls)
            self.assertIs(cls._hints, hints, cls)
            info = cls._conversionInfo()
            for key in ("returns", "implicit", "exact", "explicit", "none",
                        "attributes"):
                self.assertIsInstance(getattr(info, key), list, (cls, key))

    def testConcurrentHintsAccessIsStable(self):
        """Concurrent readers share one hints object and get complete info.

        Each _conversionInfo() call builds its own lists, so no reader can see
        another reader's half built result.
        """
        import threading
        import _jpype

        cls = _jpype.JClass("jpype.classhints.ClassHintsTest")
        barrier = threading.Barrier(8, timeout=10)
        keys = ("returns", "implicit", "exact", "explicit", "none",
                "attributes")
        results = []
        errors = []

        def work():
            try:
                barrier.wait()
                hints = cls._hints
                info = cls._conversionInfo()
                results.append(
                    (hints, tuple(len(getattr(info, key)) for key in keys)))
            except BaseException as ex:
                errors.append(ex)

        threads = [threading.Thread(target=work) for _ in range(8)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join(30)

        self.assertTrue(all(not thread.is_alive() for thread in threads))
        self.assertEqual(errors, [])
        self.assertEqual(len(results), 8)
        self.assertTrue(all(result[0] is results[0][0]
                            for result in results))
        self.assertTrue(all(result[1] == results[0][1]
                            for result in results))


class ClassCreationThreadedTest(unittest.TestCase):
    """A class must not be handed to a second thread before it is complete.

    The Python wrapper of a Java class is published before the post load
    step (customizers, inner classes) has run.  Another thread asking for
    the same class in that window used to receive a class that was missing
    its inner classes, e.g. ``DerivedTest.Base``.
    """

    def testClassIsCompleteWhenReturnedToSecondThread(self):
        script = r'''
import sys
import threading
import _jpype
import jpype

published = threading.Event()
release = threading.Event()
started = threading.Event()
finished = threading.Event()
results = {}
original_post = _jpype._jclassPost
name = "jpype.overloads.DerivedTest"

def delayed_post(cls, *args):
    if cls.__name__ == name:
        published.set()
        if not release.wait(5):
            raise RuntimeError("class initialization was not released")
    return original_post(cls, *args)

_jpype._jclassPost = delayed_post
jpype.startJVM(classpath=[sys.argv[1]])

def create():
    try:
        results["creator"] = jpype.JClass(name)
    except BaseException as ex:
        results["creator"] = ex

def lookup():
    started.set()
    try:
        results["waiter"] = jpype.JClass(name)
    except BaseException as ex:
        results["waiter"] = ex
    finally:
        finished.set()

creator = threading.Thread(target=create, daemon=True)
waiter = threading.Thread(target=lookup, daemon=True)
creator.start()
try:
    assert published.wait(5), "class wrapper was not published"
    waiter.start()
    assert started.wait(5), "lookup did not start"
    assert not finished.wait(0.2), "second thread got an unfinished class"
finally:
    release.set()
creator.join(5)
waiter.join(5)
assert not creator.is_alive() and not waiter.is_alive(), "class creation deadlock"
for key in ("creator", "waiter"):
    assert not isinstance(results[key], BaseException), results[key]
    assert hasattr(results[key], "Base"), "%s class lacks its inner classes" % key
assert results["creator"] is results["waiter"]
print("CLASS COMPLETE", flush=True)
'''
        classes = Path(__file__).resolve().parents[1] / "classes"
        result = subprocess.run(
            [sys.executable, "-c", script, str(classes)],
            capture_output=True, text=True, timeout=30,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("CLASS COMPLETE", result.stdout)

    def testConcurrentFirstUseHasInnerClasses(self):
        script = r'''
import sys
import threading
import jpype

jpype.startJVM(classpath=[sys.argv[1]])
cases = (("jpype.overloads.DerivedTest", "Base"),
         ("jpype.overloads.InterfaceOverload", "BothOverloads"))
bad = []
for name, inner in cases:
    barrier = threading.Barrier(8, timeout=10)

    def work():
        barrier.wait()
        if not hasattr(jpype.JClass(name), inner):
            bad.append(name)

    threads = [threading.Thread(target=work) for _ in range(8)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join(30)
    assert all(not thread.is_alive() for thread in threads), "deadlock"
assert not bad, bad
print("INNER CLASSES OK", flush=True)
'''
        classes = Path(__file__).resolve().parents[1] / "classes"
        for _ in range(3):
            result = subprocess.run(
                [sys.executable, "-c", script, str(classes)],
                capture_output=True, text=True, timeout=60,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("INNER CLASSES OK", result.stdout)
