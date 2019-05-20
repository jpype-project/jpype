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
import os
import jpype
from . import common

# https://gist.github.com/edufelipe/1027906#file-gistfile1-py
def check_output(*popenargs, **kwargs):
    r"""Run command with arguments and return its output as a byte string.
    Backported from Python 2.7 as it's implemented as pure python on stdlib.
    >>> check_output(['/usr/bin/python', '--version'])
    Python 2.6.2
    """
    os.environ['PYTHONPATH'] = os.getcwd()
    process = subprocess.Popen(stdout=subprocess.PIPE, *popenargs, **kwargs)
    output, unused_err = process.communicate()
    retcode = process.poll()
    if retcode:
        cmd = kwargs.get("args")
        if cmd is None:
            cmd = popenargs[0]
        error = subprocess.CalledProcessError(retcode, cmd)
        error.output = output
        raise error
    return output

class StartJVMCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testStartup(self):
        # Test that we are robust to multiple startJVM
        try:
            self.assertRaises(OSError, jpype.startJVM,
                              jpype.getDefaultJVMPath())
            self.assertRaises(OSError, jpype.startJVM,
                              jpype.getDefaultJVMPath())
        except RuntimeError:
            pass
            # Verify that we don't crash after repeat
        jpype.JClass("java.lang.String")


class TestNewJVMInstance(unittest.TestCase):
    def test_invalid_args(self):
        if sys.platform == 'cygwin':
            raise unittest.SkipTest("not tested on cygwin")
        inv_arg = '-for_sure_InVaLiD'
        script = 'import jpype; jpype.startJVM(None, "{arg}", ignoreUnrecognized=False)'.format(arg=inv_arg)
        with self.assertRaises(subprocess.CalledProcessError) as cpe:
            check_output([sys.executable, '-c', script], stderr=subprocess.STDOUT)
        exception_stdout = cpe.exception.output.decode('ascii')
        self.assertIn('Unrecognized option', exception_stdout)
        self.assertIn(inv_arg, exception_stdout)

    def test_invalid_args2(self):
        if sys.platform == 'cygwin':
            raise unittest.SkipTest("not tested on cygwin")
        inv_arg = '-for_sure_InVaLiD'
        script = 'import jpype; jpype.startJVM(None, "{arg}", ignoreUnrecognized=True)'.format(arg=inv_arg)
        check_output([sys.executable, '-c', script], stderr=subprocess.STDOUT)

    def test_classpath_arg(self):
        """ pass class path of jpypetest and try to instance a contained class.
        This only works if the classpath argument is handled correctly.
        """
        if sys.platform == 'cygwin':
            raise unittest.SkipTest("not tested on cygwin")
        import os
        root = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
        cp = os.path.join(root, 'classes').replace('\\','/')
        assert os.path.exists(cp)
        jclass = 'jpype.array.TestArray'
        path_to_test_class = os.path.join(cp, jclass.replace('.', os.path.sep) + '.class')
        assert os.path.exists(path_to_test_class)
        #script = ('import jpype; jpype.startJVM(None, classpath="{cp}"); print(jpype.java.lang.System.getProperty("java.class.path"))'
        #          .format(cp=cp))
        #print(check_output([sys.executable, '-c', script]))
        script = ('import jpype; jpype.startJVM(None, classpath="{cp}"); jpype.JClass("{jclass}")'
                  .format(cp=cp, jclass=jclass))
        check_output([sys.executable, '-c', script])
