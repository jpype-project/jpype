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

import pytest
import _jpype
import jpype
import logging
from os import path
import unittest  # Extensively used as common.unittest.

CLASSPATH = None
fast = False


def version(v):
    return tuple([int(i) for i in v.split('.')])


def requirePythonAfter(required):
    import re
    import platform
    pversion = tuple([int(re.search(r'\d+',i).group()) for i in platform.python_version_tuple()])

    def g(func):
        def f(self):
            if pversion < required:
                raise unittest.SkipTest("numpy required")
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


@pytest.mark.usefixtures("common_opts")
class JPypeTestCase(unittest.TestCase):
    def setUp(self):
        if not jpype.isJVMStarted():
            try:
                import faulthandler
                faulthandler.enable()
                faulthandler.disable()
            except:
                pass
            root = path.dirname(path.abspath(path.dirname(__file__)))
            jpype.addClassPath(path.join(root, 'classes'))
            jvm_path = jpype.getDefaultJVMPath()
            logger = logging.getLogger(__name__)
            logger.info("Running testsuite using JVM %s" % jvm_path)
            classpath_arg = "-Djava.class.path=%s"
            args = ["-ea", "-Xmx256M", "-Xms16M"]
            if self._checkjni:
                args.append("-Xcheck:jni")
            # TODO: enabling this check crashes the JVM with: FATAL ERROR in native method: Bad global or local ref passed to JNI
            # "-Xcheck:jni",
            if self._classpath:
                from pathlib import Path
                import warnings
                # This needs to be relative to run location
                jpype.addClassPath(Path(self._classpath).resolve())
                warnings.warn("using jar instead of thunks")
            if self._convertStrings:
                import warnings
                warnings.warn("using deprecated convertStrings")
            if self._jacoco:
                import warnings
                args.append(
                    "-javaagent:lib/org.jacoco.agent-0.8.5-runtime.jar=destfile=build/coverage/jacoco.exec,includes=org.jpype.*")
                warnings.warn("using JaCoCo")
            jpype.addClassPath(path.join(root, "../lib/*"))
            jpype.addClassPath(path.join(root, "jar/*"))
            classpath_arg %= jpype.getClassPath()
            args.append(classpath_arg)
            _jpype.enableStacktraces(True)
            #JPypeTestCase.str_conversion = eval(os.getenv('JPYPE_STR_CONVERSION', 'True'))
            jpype.startJVM(jvm_path, *args,
                           convertStrings=self._convertStrings)
        self.jpype = jpype.JPackage('jpype')

    def tearDown(self):
        pass

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
