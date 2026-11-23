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
import os
import subprocess
import sys
import unittest
import warnings

import _jpype
import jpype
import pytest

import common


class JPypeDeprecationWarningTestCase(unittest.TestCase):

    def testCategory(self):
        # FutureWarning so that it is shown by default; a DeprecationWarning
        # subclass would be hidden by the default ignore::DeprecationWarning.
        self.assertTrue(issubclass(jpype.JPypeDeprecationWarning, FutureWarning))
        self.assertFalse(issubclass(jpype.JPypeDeprecationWarning, DeprecationWarning))
        self.assertIs(_jpype.JPypeDeprecationWarning, jpype.JPypeDeprecationWarning)

    def testDeprecatedDecorator(self):
        @jpype._core.deprecated("bar")
        def foo():
            pass
        with self.assertWarns(jpype.JPypeDeprecationWarning):
            foo()

    def testPytestDeprecatedCall(self):
        with pytest.deprecated_call():
            warnings.warn("x", jpype.JPypeDeprecationWarning)

    def testShownByDefaultFromLibraryCode(self):
        # Warn from a module that is not __main__, in a fresh interpreter
        # without any warning filters: a DeprecationWarning would be hidden.
        code = (
            "import types, jpype\n"
            "m = types.ModuleType('somelib')\n"
            "exec('def f(g):\\n    g()', m.__dict__)\n"
            "@jpype._core.deprecated('bar')\n"
            "def old():\n"
            "    pass\n"
            "m.f(old)\n"
        )
        env = dict(os.environ)
        env.pop("PYTHONWARNINGS", None)
        res = subprocess.run([sys.executable, "-c", code], env=env,
                             capture_output=True, text=True, timeout=60)
        self.assertEqual(res.returncode, 0, res.stderr)
        self.assertIn("JPypeDeprecationWarning", res.stderr)


class JPypeDeprecationWarningJVMTestCase(common.JPypeTestCase):

    def testJObjectWithPythonType(self):
        with self.assertWarns(jpype.JPypeDeprecationWarning):
            jpype.JObject(None, object)
