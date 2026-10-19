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

class UseFunc(object):
    def __init__(self, obj, func, attr):
        self.obj = obj
        self.func = func
        self.attr = attr
        self.orig = getattr(self.obj, self.attr)

    def __enter__(self):
        setattr(self.obj, self.attr, self.func)

    def __exit__(self, exception_type, exception_value, traceback):
        setattr(self.obj, self.attr, self.orig)


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
