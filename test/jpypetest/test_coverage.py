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
import jpype.imports
import common
import sys
import os
import importlib
import pytest
from unittest import mock
import _jpype

# Tests just for coverage
# These will be moved to the corresponding test file as needed


class CoverageCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.platform = sys.platform

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
        self.assertEqual(js + "bar", "foobar")
        if not self._convertStrings:
            self.assertIsInstance(js + "bar", jpype.java.lang.String)

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
        class JArray2(jpype.JArray, internal=True):
            pass
        with self.assertRaises(TypeError):
            JArray2(jpype.JInt)

    @common.requireInstrumentation
    def testJStringFail(self):
        _jpype.fault("PyJPClass_new::verify")

        class JString2(jpype.JString, internal=True):
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

    def testHints(self):
        with self.assertRaises(AttributeError):
            jpype.JObject._hints = object()

    @pytest.mark.filterwarnings("ignore::DeprecationWarning")
    def testDeprecated(self):
        @jpype._core.deprecated
        def foo():
            pass

        @jpype._core.deprecated("foo")
        def bar():
            pass
        bar()
        foo()

    def testVersionPreStart(self):
        with mock.patch('_jpype.isStarted') as func:
            func.return_value = False
            self.assertEqual(jpype.getJVMVersion(), (0, 0, 0))

    def testGui(self):
        def foo():
            pass  # this is executed in a thread which may start later
        magic = mock.MagicMock()
        with mock.patch("sys.platform", "other"), mock.patch.dict('sys.modules', {'PyObjCTools': magic}):
            from PyObjCTools import AppHelper
            self.assertEqual(sys.platform, "other")
            jpype.setupGuiEnvironment(foo)
            self.assertFalse(magic.AppHelper.runConsoleEventLoop.called)
            jpype.shutdownGuiEnvironment()
            self.assertFalse(magic.AppHelper.stopEventLoop.called)
        with mock.patch("sys.platform", "darwin"), mock.patch.dict('sys.modules', {'PyObjCTools': magic}):
            from PyObjCTools import AppHelper
            self.assertEqual(sys.platform, "darwin")
            jpype.setupGuiEnvironment(foo)
            self.assertTrue(magic.AppHelper.runConsoleEventLoop.called)
            jpype.shutdownGuiEnvironment()
            self.assertTrue(magic.AppHelper.stopEventLoop.called)

    def testImportKey(self):
        self.assertEqual(jpype.imports._keywordUnwrap("with_"), "with")
        self.assertEqual(jpype.imports._keywordUnwrap("func"), "func")
        self.assertEqual(jpype.imports._keywordWrap("with"), "with_")
        self.assertEqual(jpype.imports._keywordWrap("func"), "func")

    def testImportNotStarted(self):
        with mock.patch('_jpype.isStarted') as func:
            func.return_value = False
            with self.assertRaisesRegex(ImportError, "jvm"):
                import mil.spec
