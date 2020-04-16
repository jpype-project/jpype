import jpype
import common
import subrun
import os
import sys
import unittest


def runStartJVM(*args, **kwargs):
    jpype.startJVM(*args, **kwargs)


def runStartJVMTest(*args, **kwargs):
    jpype.startJVM(*args, **kwargs)
    try:
        jclass = jpype.JClass('jpype.array.TestArray')
        return
    except:
        pass
    raise RuntimeError("Test class not found")


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

    def testJVMPathKeyword(self):
        runStartJVM(jvmpath=self.jvmpath)

    def testInvalidArgsFalse(self):
        with self.assertRaises(RuntimeError):
            runStartJVM("-for_sure_InVaLiD",
                        ignoreUnrecognized=False, convertStrings=False)

    def testInvalidArgsTrue(self):
        runStartJVM("-for_sure_InVaLiD",
                    ignoreUnrecognized=True, convertStrings=False)

    def testClasspathArgKeyword(self):
        runStartJVMTest(classpath=cp, convertStrings=False)

    def testClasspathArgList(self):
        runStartJVMTest(classpath=[cp], convertStrings=False)

    def testClasspathArgListEmpty(self):
        runStartJVMTest(classpath=[cp, ''], convertStrings=False)

    def testClasspathArgDef(self):
        runStartJVMTest('-Djava.class.path=%s' % cp, convertStrings=False)

    def testClasspathTwice(self):
        with self.assertRaises(TypeError):
            runStartJVMTest('-Djava.class.path=%s' %
                            cp, classpath=cp, convertStrings=False)

    def testClasspathBadType(self):
        with self.assertRaises(TypeError):
            runStartJVMTest(classpath=1, convertStrings=False)

    def testPathArg(self):
        runStartJVMTest(self.jvmpath, classpath=cp, convertStrings=False)

    def testPathKeyword(self):
        path = jpype.getDefaultJVMPath()
        runStartJVMTest(classpath=cp, jvmpath=self.jvmpath,
                        convertStrings=False)

    def testPathTwice(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(self.jvmpath, jvmpath=self.jvmpath)

    def testBadKeyword(self):
        with self.assertRaises(TypeError):
            jpype.startJVM(invalid=True)
