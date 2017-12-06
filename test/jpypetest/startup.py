# *****************************************************************************
#   Copyright 2017 Karl Einar Nelson
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
# *****************************************************************************
try:
    import unittest2 as unittest
except ImportError:
    import unittest
import subprocess
import sys
import jpype
from . import common

class StartJVMCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testStartup(self):
        # Test that we are robust to multiple startJVM
        try:
            self.assertRaises(OSError, jpype.startJVM, jpype.getDefaultJVMPath())
            self.assertRaises(OSError, jpype.startJVM, jpype.getDefaultJVMPath())
        except RuntimeError:
            pass
            # Verify that we don't crash after repeat
        jpype.JClass("java.lang.String")


class TestNewJVMInstance(unittest.TestCase):
    def test_invalid_args(self):
        inv_arg = '-for_sure_InVaLiD'
        script = 'import jpype; jpype.startJVM(None, "{}", ignoreUnrecognized=False)'.format(inv_arg)
        with self.assertRaises(subprocess.CalledProcessError) as cpe:
            subprocess.check_output([sys.executable, '-c', script], stderr=subprocess.STDOUT)
        exception_stdout = cpe.exception.stdout.decode('ascii')
        self.assertIn('Unrecognized option', exception_stdout)
        self.assertIn(inv_arg, exception_stdout)

    def test_classpath_arg(self):
        """ pass class path of jpypetest and try to instance a contained class.
        This only works if the classpath argument is handled correctly.
        """
        import os
        root = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
        cp = os.path.join(root, 'classes')
        assert os.path.exists(cp)
        jclass = 'jpype.array.TestArray'
        script = ('import jpype; jpype.startJVM(None, classpath="{cp}"); jpype.JClass({jclass})'
                  .format(cp=cp, jclass=jclass))
        subprocess.check_call([sys.executable, '-c', script])
