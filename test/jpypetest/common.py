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
from functools import lru_cache

try:
    import pytest
except ImportError:
    # Not bundled on Android (see project/android/testapp/main.py: the
    # ported suite runs under plain unittest there, avoiding the open
    # question of whether pytest itself builds/runs on Android at all).
    # Only JPypeTestCase's `@pytest.mark.usefixtures("jvm_session")` below
    # needs it, and that fixture has nothing to do on Android anyway - the
    # JVM is already running and attached by the time `import jpype`
    # returns (see jpype/__init__.py's `_jpype.bootstrap()` call).
    pytest = None  # type: ignore[assignment]
import jpype
from os import path
import threading
import unittest  # Extensively used as common.unittest.

CLASSPATH = None
fast = False


def version(v):
    return tuple([int(i) for i in v.split('.')])


def isAndroid():
    # ANDROID_ARGUMENT is set by p4a's own bootstrap (PythonActivity.java's
    # nativeSetenv call) before Python starts - the standard way p4a/Kivy
    # apps detect they're running under python-for-android, rather than a
    # plain desktop interpreter that happens to import this same file.
    import os
    return 'ANDROID_ARGUMENT' in os.environ


def skipOnAndroid(reason):
    """Gate a test method on an Android platform limitation (see
    doc/android.rst's "Removed JPype Services" and "Unsupported Java
    libraries"). A no-op everywhere else, so the same test file runs
    unmodified on both platforms."""
    def deco(func):
        return unittest.skipIf(isAndroid(), reason)(func)
    return deco


def threadUnsafe(obj):
    """Mark a test (or a whole TestCase class) as not safe to run concurrently.

    The free-threaded CI legs run every test body in several threads at the
    same time (pytest-run-parallel), on one shared TestCase instance.  Tests
    that change process wide state (class customizers and conversions, class
    path, one shot JVM class initialization, sys.stdout, ...), that share
    mutable Java fixtures through ``self``, or that measure memory must opt
    out.  A no-op where pytest is not available (Android).
    """
    if pytest is not None:
        return pytest.mark.thread_unsafe(obj)
    return obj


#: Serializes tests that write a static field of a shared Java fixture and
#: read it back.  Static fields exist once per JVM, so a per-thread fixture
#: instance (see perThread) cannot isolate them.
staticFixtureLock = threading.RLock()


class perThread(object):
    """Descriptor giving every thread its own lazily created value.

    pytest-run-parallel runs the body of one test in several threads on a
    single TestCase instance, so anything the test mutates through ``self``
    (typically a Java ``Fixture`` object) is shared by all of them.  Declared
    on the class as ``fixture = common.perThread(factory)``, each thread gets
    a separate instance, created on first use by ``factory``; assignments such
    as ``self.fixture = x`` in setUp are stored for the assigning thread only.
    Values are per test instance, so tests never see each other's state.
    """

    def __init__(self, factory):
        self.factory = factory
        self.name = None

    def __set_name__(self, owner, name):
        self.name = "_perthread_" + name

    def __get__(self, obj, owner=None):
        if obj is None:
            return self
        store = obj.__dict__.setdefault(self.name, {})
        tid = threading.get_ident()
        try:
            return store[tid]
        except KeyError:
            return store.setdefault(tid, self.factory())

    def __set__(self, obj, value):
        obj.__dict__.setdefault(self.name, {})[threading.get_ident()] = value


def requirePythonAfter(required):
    import re
    import platform
    pversion = tuple([int(re.search(r'\d+',i).group()) for i in platform.python_version_tuple()])

    def g(func):
        def f(self):
            if pversion < required:
                raise unittest.SkipTest("newer python required")
            return func(self)
        return f
    return g


def requireInstrumentation(func):
    def f(self):
        import _jpype
        if not hasattr(_jpype, "fault"):
            raise unittest.SkipTest("instrumentation required")
        rc = func(self)
        _jpype.fault(None)
        return rc
    return f


def requireNumpy(func):
    def f(self):
        try:
            import numpy
            return func(self)
        except ImportError:
            pass
        raise unittest.SkipTest("numpy required")
    return f

def requireAscii(func):
    if isAndroid():
        # No equivalent "source root" to check against on Android (the
        # app's own data directory is always ASCII in practice), so this
        # is simply a no-op pass-through rather than a real check.
        return func

    def f(self):
        try:
            root = path.dirname(path.abspath(path.dirname(__file__)))
            if root.isascii():
                return func(self)
        except ImportError:
            raise unittest.SkipTest("Ascii root directory required")
    return f

_useFuncLock = threading.Lock()


class _UseFuncDispatch(object):
    """Stands in for a replaced method; each thread sees its own override."""

    def __init__(self, orig):
        self.orig = orig
        self.local = threading.local()
        self.users = 0

    def __call__(self, *args, **kwargs):
        stack = getattr(self.local, "stack", None)
        return (stack[-1] if stack else self.orig)(*args, **kwargs)


class UseFunc(object):
    """Temporarily replace a method (e.g. assertEqual) of a test case.

    The replacement is only visible to the thread that entered the ``with``
    block.  A test body may be executed by several threads on the same
    TestCase instance (pytest-run-parallel), so swapping the attribute for
    everybody would let one thread's exit restore the original while another
    thread is still inside its block.  The original is put back once the last
    thread has left.
    """

    def __init__(self, obj, func, attr):
        self.obj = obj
        self.func = func
        self.attr = attr
        self.key = "_usefunc_" + attr
        self.dispatch = None

    def __enter__(self):
        with _useFuncLock:
            d = self.obj.__dict__.get(self.key)
            if d is None:
                d = _UseFuncDispatch(getattr(self.obj, self.attr))
                self.obj.__dict__[self.key] = d
                setattr(self.obj, self.attr, d)
            d.users += 1
        self.dispatch = d
        if not hasattr(d.local, "stack"):
            d.local.stack = []
        d.local.stack.append(self.func)

    def __exit__(self, exception_type, exception_value, traceback):
        d = self.dispatch
        d.local.stack.pop()
        with _useFuncLock:
            d.users -= 1
            if d.users == 0:
                setattr(self.obj, self.attr, d.orig)
                del self.obj.__dict__[self.key]


class JPypeTestCase(unittest.TestCase):
    if pytest is not None:
        # Equivalent to decorating this class with
        # @pytest.mark.usefixtures("jvm_session") (pytest reads either
        # form the same way) - written this way, rather than as a
        # decorator, so the class definition itself stays a plain,
        # unconditional `class JPypeTestCase(unittest.TestCase):` for
        # mypy/every subclass's benefit; only this one attribute is
        # conditional on pytest being importable (not the case on
        # Android - see the try/import above - where there is nothing
        # for this fixture to do anyway, the JVM is already attached by
        # the time `import jpype` returns).
        pytestmark = pytest.mark.usefixtures("jvm_session")

    def setUp(self):
        self.jpype = jpype.JPackage('jpype')

    def assertElementsEqual(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
            self.assertEqual(a[i], b[i])

    def assertElementsAlmostEqual(self, a, b, places=None, msg=None,
                          delta=None):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
            self.assertAlmostEqual(a[i], b[i], places, msg, delta)

    def useEqualityFunc(self, func):
        return UseFunc(self, func, 'assertEqual')


@lru_cache(1)
def java_version():
    import subprocess
    import sys
    java_version = str(subprocess.check_output([sys.executable, "-c",
                          "import jpype; jpype.startJVM(); "
                          "print(jpype.java.lang.System.getProperty('java.version'))"]),
                       encoding='ascii')
    # todo: make this robust for version "numbers" containing strings (e.g.) 22.1-internal
    return tuple(map(int, java_version.split(".")))
