import jpype
import common
import sys
import os
import importlib
import _jpype

# Tests just for coverage
# These will be moved to the corresponding test file as needed


class CoverageCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.platform = sys.platform

    def testJInterator(self):
        f = jpype.JIterator("foo")
        self.assertEqual(f, "foo")

    def testWin32(self):
        if sys.platform == "win32":
            raise common.unittest.SkipTest("not tested on win32")
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
            jpype.JArray(jpype.JInt, 2, 2)

    def testJArrayFail2(self):
        with self.assertRaises(TypeError):
            jpype.JArray(jpype.JInt, 1)(1, 2, 3)

    def testJArrayStr(self):
        self.assertEqual(str(jpype.JArray(jpype.JInt)([1, 2])), str([1, 2]))

    def testJArrayLength(self):
        ja = jpype.JArray(jpype.JInt)([1, 2])
        self.assertEqual(ja.length, len(ja))

    def testJArrayGetItemSlice(self):
        with self.assertRaises(NotImplementedError):
            ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4])
            ja[0:2:-1]

    def testJArraySetItemSlice(self):
        ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4, 5, 6])
        ja[0:-1:2] = [-1, -1, -1]
        self.assertEqual(list(ja[:]), [-1, 2, -1, 4, -1, 6])
        # FIXME ja[0:-1:2] = [-1] works but should not
        # FIXME ja[0:-1:2] = 1 gives wierd error

    def testJArrayGetItemSlice(self):
        ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4])
        ja[1:]

    def testJArraySetItemSlice(self):
        ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4])
        ja[1:] = [3, 4, 5]
        self.assertEqual(list(ja[:]), [1, 3, 4, 5])

    def testJArrayEQ(self):
        ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4])
        ja == [1, 2]

    def testJArrayNE(self):
        ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4])
        ja != [1, 2]

    def testJArrayIter(self):
        ja = jpype.JArray(jpype.JInt)([1, 2, 3, 4])
        for i in ja:
            pass

    def testJArrayBytes(self):
        ja = jpype.JArray(jpype.JByte)([65, 66])
        self.assertEqual(str(ja), "AB")

    def testJArrayBytesFail(self):
        ja = jpype.JArray(jpype.JByte)([65, 66])
        self.assertFalse(ja == None)

    def testJBooleanFail(self):
        with self.assertRaises(TypeError):
            jpype.java.lang.Boolean(1, 2)

    def testJStringAppend(self):
        js = jpype.JString("foo")
        self.assertEqual(js+"bar", "foobar")
        if not self._convertStrings:
            self.assertIsInstance(js+"bar", jpype.java.lang.String)

    def testJStringNE(self):
        js = jpype.JString("foo")
        self.assertFalse(js != "foo")
        self.assertFalse(js != jpype.JString("foo"))

    # FIXME review this for slices and other cases, we may need
    # to improve this one
    def testJStringGetItem(self):
        js = jpype.JString("fred")
        self.assertEqual(js[1], "r")

    def testJStringLen(self):
        js = jpype.JString("fred")
        self.assertEqual(len(js), 4)

    def testJStringLT(self):
        js = jpype.JString("b")
        self.assertTrue(js < 'c')
        self.assertFalse(js < 'b')
        self.assertFalse(js < 'a')

    def testJStringLE(self):
        js = jpype.JString("b")
        self.assertTrue(js <= 'c')
        self.assertTrue(js <= 'b')
        self.assertFalse(js <= 'a')

    def testJStringGT(self):
        js = jpype.JString("b")
        self.assertFalse(js > 'c')
        self.assertFalse(js > 'b')
        self.assertTrue(js > 'a')

    def testJStringGE(self):
        js = jpype.JString("b")
        self.assertFalse(js >= 'c')
        self.assertTrue(js >= 'b')
        self.assertTrue(js >= 'a')

    def testJStringContains(self):
        js = jpype.JString("fred")
        self.assertTrue("r" in js)
        self.assertFalse("g" in js)

    def testJStringRepr(self):
        js = jpype.JString("fred")
        self.assertTrue(repr(js), "fred")

#    def testSetResourceFail(self):
#        with self.assertRaises(RuntimeError):
#            _jpype.setResource("NotAResource", None)

    # FIXME this one is broken
#    def testJPrimitiveSetAttr(self):
#        ji = jpype.JInt(1)
#        with self.assertRaises(AttributeError):
#            ji.qq = "wow"

    # FIXME this path seems like something outdated and is not working
#    def testJBooleanFromBool(self):
#        self.assertTrue(jpype.java.lang.Boolean(jpype.JInt(40))==True)

    def testJArrayFail(self):
        class JArray2(jpype.JArray):
            pass
        with self.assertRaises(TypeError):
            JArray2(jpype.JInt)

    def testJStringFail(self):
        class JString2(jpype.JString):
            pass
        with self.assertRaises(TypeError):
            JString2("foo")

    def testCustomizerLate(self):
        with self.assertRaises(TypeError):
            @jpype.JImplementationFor("java.lang.Object", base=True)
            class Sally(object):
                pass

    def testCustomizerBadType(self):
        with self.assertRaises(TypeError):
            @jpype.JImplementationFor({})
            class Sally(object):
                pass

    def testModuleHasClass(self):
        self.assertTrue(_jpype._hasClass("java.lang.Object"))

    def testModuleExamine(self):
        # this is an internal testing routine for Java slots
        _jpype.examine(jpype.JString)
        _jpype.examine(jpype.JString("foo"))

    def testJClassBadClass(self):
        with self.assertRaises(Exception):
            jpype.JClass("not.a.class")

    def testJClassBadType(self):
        with self.assertRaises(TypeError):
            jpype.JClass({})

    def testJClassFromClass(self):
        self.assertIsInstance(jpype.JClass(jpype.java.lang.Class.forName(
            "java.lang.StringBuilder")), jpype.JClass)
