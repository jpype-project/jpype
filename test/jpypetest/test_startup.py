import jpype
import common
import subrun
import os
import sys

def runStartupTwice():
    jpype.startJVM(convertStrings=False)
    jpype.startJVM(convertStrings=False)

def runRestart():
    jpype.startJVM(convertStrings=False)
    jpype.shutdownJVM()
    jpype.startJVM(convertStrings=False)


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

class StartJVMCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.path = jpype.getDefaultJVMPath()
        root = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
        cp = os.path.join(root, 'classes').replace('\\','/')
        assert os.path.exists(cp)
        self.cp = cp

    def testStartup(self):
        with subrun.Client() as client, self.assertRaises(OSError):
            client.execute(runStartupTwice)

    def testRestart(self):
        with subrun.Client() as client, self.assertRaises(OSError):
            client.execute(runRestart)

    def testJVMPathKeyword(self):
        with subrun.Client() as client:
            client.execute(runStartJVM, jvmpath=self.path)

    def testInvalidArgsFalse(self):
        with subrun.Client() as client, self.assertRaises(RuntimeError):
            client.execute(runStartJVM, "-for_sure_InVaLiD", ignoreUnrecognized=False, convertStrings=False)

    def testInvalidArgsTrue(self):
        with subrun.Client() as client:
            client.execute(runStartJVM, "-for_sure_InVaLiD", ignoreUnrecognized=True, convertStrings=False)

    def testClasspathArgKeyword(self):
        with subrun.Client() as client:
            client.execute(runStartJVMTest, classpath=self.cp, convertStrings=False)

    def testClasspathArgList(self):
        with subrun.Client() as client:
            client.execute(runStartJVMTest, classpath=[self.cp], convertStrings=False)

    def testClasspathArgListEmpty(self):
        with subrun.Client() as client:
            client.execute(runStartJVMTest, classpath=[self.cp,''], convertStrings=False)

    @common.unittest.skipIf(sys.platform=="cygwin", "Not supported on cygwin")
    def testClasspathArgDef(self):
        with subrun.Client() as client:
            client.execute(runStartJVMTest, '-Djava.class.path=%s'%self.cp, convertStrings=False)

    def testClasspathTwice(self):
        with subrun.Client() as client, self.assertRaises(TypeError):
            client.execute(runStartJVMTest, '-Djava.class.path=%s'%self.cp, classpath=self.cp, convertStrings=False)

    def testClasspathBadType(self):
        with subrun.Client() as client, self.assertRaises(TypeError):
            client.execute(runStartJVMTest, classpath=1, convertStrings=False)

    def testPathArg(self):
        with subrun.Client() as client:
            client.execute(runStartJVMTest, self.path, classpath=self.cp, convertStrings=False)

    def testPathKeyword(self):
        with subrun.Client() as client:
            client.execute(runStartJVMTest, classpath=self.cp, jvmpath=self.path, convertStrings=False)

    def testPathTwice(self):
        with subrun.Client() as client, self.assertRaises(TypeError):
            client.execute(runStartJVM, self.path, jvmpath=self.path)

    def testBadKeyword(self):
        with subrun.Client() as client, self.assertRaises(TypeError):
            client.execute(runStartJVM, invalid=True)



