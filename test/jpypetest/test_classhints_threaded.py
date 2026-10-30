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
"""Regressions for concurrent first use of class conversion hints (#1507)."""
from pathlib import Path
import subprocess
import sys
import unittest
import pytest

pytestmark = pytest.mark.thread_unsafe

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
    assert any(isinstance(ex, RuntimeError) for ex in errors), errors
    assert any(isinstance(ex, TypeError) for ex in errors), errors
    assert not converted.is_set(), "conversion used a failed class"
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
