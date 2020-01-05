import jpype
import common
import sys
import os
import importlib

# Tests just for coverage 
# These will be moved to the corresponding test file as needed
class CoverageCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.platform = sys.platform

    def testJInterator(self):
        f = jpype.JIterator("foo")
        self.assertEqual(f, "foo")

    def testCygwin(self):
        try:
            sys.platform = "cygwin"
            importlib.reload(jpype._classpath)
            self.assertEqual(jpype._classpath._SEP, ";")
            self.assertEqual(jpype._classpath._root, None)
            jpype._classpath._root = r'C:\cygwin64'
            self.assertEqual(jpype._classpath._get_root(), r'C:\cygwin64')
            self.assertEqual(jpype._classpath._splitpath('/cygdrive/c/cygwin64'), ['cygdrive','c','cygwin64'])
            self.assertEqual(jpype._classpath._posix2win('/cygdrive/c/windows'), 'c:\\windows')
            self.assertEqual(jpype._classpath._posix2win('/bin'), r"C:\cygwin64\bin")
            jpype._classpath._CLASSPATHS = []
            jpype.addClassPath("/cygdrive/c/windows")
            jpype.addClassPath("/cygdrive/c/data")
            self.assertEqual(jpype._classpath._CLASSPATHS, [r"c:\windows", r"c:\data"])
            env = os.environ.get("CLASSPATH")
            os.environ["CLASSPATH"]=r"c:\programs"
            self.assertEqual(jpype.getClassPath(True), r"c:\windows;c:\data;c:\programs")
            self.assertEqual(jpype.getClassPath(False), r"c:\windows;c:\data")
            #jpype.addClassPath("")
            #self.assertEqual(jpype.getClassPath(False), r"c:\windows;c:\data")
            jpype.addClassPath("/cygdrive/c/data/*")
            self.assertEqual(jpype.getClassPath(False), r"c:\windows;c:\data")
            if not env:
                del os.environ["CLASSPATH"]
            jpype._classpath._CLASSPATHS = []

            with self.assertRaises(jpype.JVMNotFoundException):
                jpype.getDefaultJVMPath()

        finally:
            sys.platform = self.platform
            importlib.reload(jpype._classpath)

    def testWin32(self):
        try:
            sys.platform = "win32"
            importlib.reload(jpype._classpath)
            with self.assertRaises(jpype.JVMNotFoundException):
                jpype.getDefaultJVMPath()

        finally:
            sys.platform = self.platform
            importlib.reload(jpype._classpath)
 
    def testHandleClassPath(self):
        with self.assertRaises(TypeError):
            jpype._core._handleClassPath([1])
        jpype._core._handleClassPath(["./*.jar"])

    def testRestart(self):
        with self.assertRaises(OSError):
            jpype.startJVM()

    def testSynchronizeFail(self):
        with self.assertRaises(TypeError):
            jpype.synchronized("hello")

    def testJVMPathDepecated(self):
        jpype.get_default_jvm_path()

    def testJArrayFail1(self):
        with self.assertRaises(TypeError):
            jpype.JArray(jpype.JInt,2,2)

    def testJArrayFail2(self):
        with self.assertRaises(TypeError):
            jpype.JArray(jpype.JInt,1)(1,2,3)

    def testJArrayStr(self):
        self.assertEqual(str(jpype.JArray(jpype.JInt)([1,2])), str((1,2)))

    def testJArrayLength(self):
        ja = jpype.JArray(jpype.JInt)([1,2])
        self.assertEqual(ja.length, len(ja))

    def testJArrayGetItemSlice(self):
        with self.assertRaises(NotImplementedError):
            ja = jpype.JArray(jpype.JInt)([1,2,3,4])
            ja[0:2:-1]

    def testJArraySetItemSlice(self):
        ja = jpype.JArray(jpype.JInt)([1,2,3,4,5,6])
        ja[0:-1:2] = [-1,-1,-1]
        self.assertEqual(list(ja[:]),[-1,2,-1,4,-1,6])
        # FIXME ja[0:-1:2] = [-1] works but should not
        # FIXME ja[0:-1:2] = 1 gives wierd error

    def testJArrayGetItemSlice(self):
        ja = jpype.JArray(jpype.JInt)([1,2,3,4])
        ja[1:]

    def testJArraySetItemSlice(self):
        ja = jpype.JArray(jpype.JInt)([1,2,3,4])
        ja[1:]=[3,4,5]
        self.assertEqual(list(ja[:]),[1,3,4,5])

    def testJArrayEQ(self):
        ja = jpype.JArray(jpype.JInt)([1,2,3,4])
        ja==[1,2]

    def testJArrayNE(self):
        ja = jpype.JArray(jpype.JInt)([1,2,3,4])
        ja!=[1,2]

    def testJArrayIter(self):
        ja = jpype.JArray(jpype.JInt)([1,2,3,4])
        for i in ja:
            pass

    def testJArrayBytes(self):
        ja = jpype.JArray(jpype.JByte)([65,66])
        self.assertEqual(str(ja), "AB")

    def testJArrayBytesFail(self):
        ja = jpype.JArray(jpype.JByte)([65,66])
        self.assertFalse(ja==None)

    def testJBooleanFail(self):
        with self.assertRaises(TypeError):
            jpype.java.lang.Boolean(1,2)

    # FIXME this path seems like something outdated and is not working
#    def testJBooleanFromBool(self):
#        class A:
#           def booleanValue(self):
#                return True
#        self.assertTrue(jpype.java.lang.Boolean(A())==True)


















