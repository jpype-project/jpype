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
import jpype
import subrun
import functools
import os
from pathlib import Path
import unittest


@functools.wraps(jpype.startJVM)
def runStartJVMTest(*args, **kwargs):
    jpype.startJVM(*args, **kwargs)
    try:
        assert jpype.JClass('jpype.array.TestArray') is not None
    except Exception as err:
        raise RuntimeError("Test class not found") from err


root = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
cp = os.path.join(root, 'classes').replace('\\', '/')


@subrun.TestCase(individual=True)
class StartJVMCase(unittest.TestCase):
    def setUp(self):
        self.jvmpath = jpype.getDefaultJVMPath()

    def testStartup(self):
        with self.assertRaises(OSError):
            jpype.startJVM(convertStrings=False)
            jpype.startJVM(convertStrings=False)

    def testRestart(self):
        with self.assertRaises(OSError):
            jpype.startJVM(convertStrings=False)
            jpype.shutdownJVM()
            jpype.startJVM(convertStrings=False)

    def testInvalidArgsFalse(self):
        with self.assertRaises(RuntimeError):
            jpype.startJVM(
                "-for_sure_InVaLiD",
                ignoreUnrecognized=False, convertStrings=False,
            )

    def testInvalidArgsTrue(self):
        jpype.startJVM(
            "-for_sure_InVaLiD",
            ignoreUnrecognized=True, convertStrings=False,
        )

    def testClasspathArgKeyword(self):
        runStartJVMTest(classpath=cp, convertStrings=False)

    def testClasspathArgList(self):
        runStartJVMTest(classpath=[cp], convertStrings=False)

    def testClasspathArgListEmpty(self):
        runStartJVMTest(classpath=[cp, ''], convertStrings=False)

    def testClasspathArgDef(self):
        runStartJVMTest('-Djava.class.path=%s' % cp, convertStrings=False)

    def testClasspathArgPath(self):
        runStartJVMTest(classpath=Path(cp), convertStrings=False)

    def testClasspathArgPathList(self):
        runStartJVMTest(classpath=[Path(cp)], convertStrings=False)

    def testClasspathArgGlob(self):
        jpype.startJVM(classpath=os.path.join(cp, '..', 'jar', 'mrjar*'))
        assert jpype.JClass('org.jpype.mrjar.A') is not None

    def testClasspathTwice(self):
        with self.assertRaises(TypeError):
            runStartJVMTest('-Djava.class.path=%s' %
                            cp, classpath=cp, convertStrings=False)

    def testClasspathBadType(self):
        with self.assertRaises(TypeError):
            runStartJVMTest(classpath=1, convertStrings=False)

    def testJVMPathArg_Str(self):
        runStartJVMTest(self.jvmpath, classpath=cp, convertStrings=False)

    def testJVMPathArg_None(self):
        # It is allowed to pass None as a JVM path
        runStartJVMTest(None, classpath=cp, )

    def testJVMPathArg_NoArgs(self):
        runStartJVMTest(classpath=cp)

    def testJVMPathArg_Path(self):
        with self.assertRaises(TypeError):
            runStartJVMTest(
                # Pass a path as the first argument. This isn't supported (this is
                # reflected in the type definition), but the fact that it "works"
                # gives rise to this test.
                Path(self.jvmpath),   # type: ignore
                classpath=cp,
                convertStrings=False,
            )

    def testJVMPathKeyword_str(self):
        runStartJVMTest(classpath=cp, jvmpath=self.jvmpath,
                        convertStrings=False)

    def testJVMPathKeyword_Path(self):
        runStartJVMTest(jvmpath=Path(self.jvmpath), classpath=cp, convertStrings=False)

    def testPathTwice(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(self.jvmpath, jvmpath=self.jvmpath)

    def testBadKeyword(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(invalid=True)
