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
import subprocess
import sys
import unittest


def _run(script, *flags):
    # The lazy import syntax and global mode only exist on 3.15+, and the
    # global mode is process-wide, so each case runs in its own interpreter.
    return subprocess.run([sys.executable, *flags, "-c", script],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=120)


@unittest.skipIf(sys.version_info < (3, 15), "PEP 810 lazy imports require 3.15")
class LazyImportsTestCase(unittest.TestCase):

    def testGlobalLazyMode(self):
        # Under -X lazy_imports=all, jpype's own side-effect imports must
        # still run, otherwise startJVM finds its resources missing.
        script = (
            "import sys\n"
            "assert sys.get_lazy_imports() == 'all'\n"
            "import jpype\n"
            "jpype.startJVM()\n"
            "ArrayList = jpype.JClass('java.util.ArrayList')\n"
            "a = ArrayList()\n"
            "a.add(1)\n"
            "assert len(a) == 1\n"
            "assert jpype.JArray(jpype.JInt)([1, 2])[1] == 2\n"
            "print('LAZY-ALL-OK')\n"
        )
        result = _run(script, "-X", "lazy_imports=all")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"LAZY-ALL-OK", result.stdout)

    def testGlobalLazyModeImportHook(self):
        # "import jpype.imports" is deferred under -X lazy_imports=all, so
        # startJVM must install the hook itself.
        script = (
            "import jpype\n"
            "import jpype.imports\n"
            "jpype.startJVM()\n"
            "from java.util import ArrayList\n"
            "assert ArrayList is jpype.JClass('java.util.ArrayList')\n"
            "print('LAZY-HOOK-OK')\n"
        )
        result = _run(script, "-X", "lazy_imports=all")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"LAZY-HOOK-OK", result.stdout)

    def testGlobalLazyModeImportHookAfterStart(self):
        # Importing jpype.imports after startJVM needs the name referenced
        # once to load it, as documented.
        script = (
            "import jpype\n"
            "jpype.startJVM()\n"
            "import jpype.imports\n"
            "jpype.imports\n"
            "from java.util import ArrayList\n"
            "assert ArrayList is jpype.JClass('java.util.ArrayList')\n"
            "print('LAZY-HOOK-AFTER-OK')\n"
        )
        result = _run(script, "-X", "lazy_imports=all")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"LAZY-HOOK-AFTER-OK", result.stdout)

    def testLazyFromBeforeStart(self):
        # A lazy import of a Java class may be declared before the JVM is
        # started; it resolves through jpype.imports on first use.
        script = (
            "import jpype\n"
            "import jpype.imports\n"
            "lazy from java.lang import String\n"
            "lazy import java.util as ju\n"
            "jpype.startJVM()\n"
            "assert String('abc').length() == 3\n"
            "assert ju.ArrayList is jpype.JClass('java.util.ArrayList')\n"
            "print('LAZY-FROM-OK')\n"
        )
        result = _run(script)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"LAZY-FROM-OK", result.stdout)

    def testLazyMissingClass(self):
        # A missing class is reported when the name is first used.
        script = (
            "import jpype\n"
            "import jpype.imports\n"
            "lazy from java.lang import NoSuchClass\n"
            "jpype.startJVM()\n"
            "try:\n"
            "    NoSuchClass\n"
            "except ImportError:\n"
            "    print('LAZY-MISSING-OK')\n"
        )
        result = _run(script)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"LAZY-MISSING-OK", result.stdout)
