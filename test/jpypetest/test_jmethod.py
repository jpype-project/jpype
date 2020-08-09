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
import sys
import _jpype
import jpype
from jpype.types import *
import common
import types
import functools
import inspect

# Code from stackoverflow
# Reference http://stackoverflow.com/questions/13503079/how-to-create-a-copy-of-a-python-function


def copy_func(f):
    """Based on http://stackoverflow.com/a/6528148/190597 (Glenn Maynard)"""
    g = types.FunctionType(f.__code__, f.__globals__, name=f.__name__,
                           argdefs=f.__defaults__,
                           closure=f.__closure__)
    g.__kwdefaults__ = f.__kwdefaults__
    g = functools.update_wrapper(g, f)
    return g


class JMethodTestCase(common.JPypeTestCase):
    """ Test for methods of JMethod (_jpype._JMethod)

    This should test how well the object matchs a Python3 function.
     * __self__: should appear on a bound, None otherwise.
     * __name__: should be set
     * __qualname__: should be set
     * __doc__: should be set
     * __annotations__: should give return type
     * __defaults__, __kwdefaults__, __code__, __globals__, __closure__: should
        be enough to "clone" the method.

    It should also be callable as a method, class method.

    Further inspect should work
     * inspect.getdoc() should match __doc__
     * inspect.signature() should work
     * inspect.isroutine() should be True

     We are not going to try to pretend to be Python2.
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = jpype.JClass('java.lang.String')
        self.obj = self.cls('foo')

    def testMethodSelf(self):
        self.assertEqual(self.cls.substring.__self__, None)
        self.assertEqual(self.obj.substring.__self__, self.obj)

    def testMethodName(self):
        self.assertEqual(self.cls.substring.__name__, "substring")
        self.assertEqual(self.obj.substring.__name__, "substring")

    def testMethodQualName(self):
        self.assertEqual(self.cls.substring.__qualname__,
                         "java.lang.String.substring")
        self.assertEqual(self.obj.substring.__qualname__,
                         "java.lang.String.substring")

    def testMethodDoc(self):
        self.assertIsInstance(self.cls.substring.__doc__, str)
        self.assertIsInstance(self.obj.substring.__doc__, str)
        d = self.cls.substring.__doc__
        self.cls.substring.__doc__ = None
        self.assertIsNone(self.cls.substring.__doc__)
        self.assertIsNone(self.obj.substring.__doc__)
        self.cls.substring.__doc__ = d

    def testMethodInspectDoc(self):
        self.assertIsInstance(inspect.getdoc(self.cls.substring), str)
        self.assertIsInstance(inspect.getdoc(self.obj.substring), str)
        self.assertIsInstance(inspect.getdoc(self.obj.format), str)

    def testMethodAnnotations(self):
        self.assertIsInstance(self.cls.substring.__annotations__, dict)
        self.assertIsNotNone(self.obj.substring.__annotations__)
        a = self.cls.substring.__annotations__
        d = {}
        self.cls.substring.__annotations__ = d
        self.assertEqual(self.cls.substring.__annotations__, d)
        self.cls.substring.__annotations__ = a
        self.assertIsNotNone(self.cls.substring.__annotations__)

        # This one will need to change in Python 3.8
        self.assertEqual(self.cls.substring.__annotations__[
                         "return"], self.cls)
        self.assertEqual(self.cls.trim.__annotations__[
                         "return"], self.cls)
        self.assertEqual(self.cls.getBytes.__annotations__, {})

    def testMethodInspectSignature(self):
        self.assertIsInstance(inspect.signature(
            self.cls.substring), inspect.Signature)
        self.assertIsInstance(inspect.signature(
            self.obj.substring), inspect.Signature)
        self.assertEqual(inspect.signature(
            self.obj.substring).return_annotation, self.cls)

    def testMethodInspectFunction(self):
        self.assertTrue(inspect.isfunction(self.cls.substring))
        self.assertTrue(inspect.isfunction(self.obj.substring))

    def testMethodInspectRoutine(self):
        self.assertTrue(inspect.isroutine(self.cls.substring))
        self.assertTrue(inspect.isroutine(self.obj.substring))

    def testMethodClassCall(self):
        self.assertEqual(self.cls.substring(self.obj, 1), "oo")

    def testMethodClassCallWierd(self):
        self.assertEqual(self.cls.substring("foo", 1), "oo")

    def testMethodClassCallFail(self):
        with self.assertRaises(TypeError):
            self.cls.substring(1, 1)

    def testMethodCall(self):
        self.assertEqual(self.obj.substring(1), "oo")

    def testMethodClone(self):
        a = copy_func(self.cls.substring)
        self.assertEqual(a(self.obj, 1), "oo")
        a = copy_func(self.obj.substring)
        self.assertEqual(a(1), "oo")

    def testMethodDump(self):
        # This is replaced by doc, should be removed
        self.assertIsInstance(jpype.JString("foo").substring.dump(), str)

    def testMethodDump(self):
        # This is replaced by doc, should be removed (or do something useful)
        self.assertIsInstance(jpype.JString(
            "foo").substring.matchReport(), str)

    def testMethodHelp(self):
        import io
        import contextlib
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            help(jpype.JString("a").substring)
        s = f.getvalue()
        self.assertTrue("Java method dispatch" in s)
        self.assertTrue("substring(int)" in s)
        self.assertTrue("substring(int, int)" in s)

    @common.requireInstrumentation
    def testJMethod_get(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_get")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt(1)

    @common.requireInstrumentation
    def testJMethod_str(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_get")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(fixture.callInt)

    @common.requireInstrumentation
    def testJMethod_str(self):
        Fixture = JClass("jpype.common.Fixture")
        fixture = Fixture()
        _jpype.fault("PyJPMethod_get")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(fixture.callInt)
        repr(Fixture.callInt)
        repr(fixture.callInt)

    @common.requireInstrumentation
    def testJMethod_selfFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_getSelf")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.__self__

    @common.requireInstrumentation
    def testJMethod_nameFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_getName")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.__name__

    @common.requireInstrumentation
    def testJMethod_qualnameFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_getQualName")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.__qualname__

    @common.requireInstrumentation
    def testJMethod_annotationsFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_getAnnotations")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.__annotations__

    @common.requireInstrumentation
    def testJMethod_docFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_getDoc")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.__doc__
        _jpype.fault("PyJPMethod_setDoc")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.__doc__ = None

    @common.requireInstrumentation
    def testJMethod_docFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_getCodeAttr")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callFloat.__code__

    @common.requireInstrumentation
    def testJMethod_beansFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_isBeanAccessor")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt._isBeanAccessor()
        _jpype.fault("PyJPMethod_isBeanMutator")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt._isBeanMutator()

    @common.requireInstrumentation
    def testJMethod_diagnosticsFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_matchReport")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt.matchReport()

    @common.requireInstrumentation
    def testJMethod_callFault(self):
        fixture = JClass("jpype.common.Fixture")()
        _jpype.fault("PyJPMethod_call")
        with self.assertRaisesRegex(SystemError, "fault"):
            fixture.callInt(1)

    def testJMethod_self(self):
        Fixture = JClass("jpype.common.Fixture")
        fixture = JClass("jpype.common.Fixture")()
        self.assertEqual(fixture.callInt.__self__, fixture)
        self.assertEqual(Fixture.callStaticInt.__self__, None)

    def testJMethod_name(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertIsInstance(fixture.callInt.__name__, str)
        self.assertEqual(fixture.callInt.__name__, 'callInt')

    def testJMethod_doc(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertIsInstance(fixture.callInt.__doc__, str)

    def testJMethod_annotations(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertIsInstance(fixture.callInt.__annotations__, dict)
        ann = fixture.callInt.__annotations__
        expected = {'arg0': JInt, 'return': JInt}
        self.assertEqual(ann, expected)

    def testJMethod_closure(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertNotEqual(fixture.callInt.__closure__, None)

    def testJMethod_code(self):
        fixture = JClass("jpype.common.Fixture")()

        def f():
            pass
        self.assertIsInstance(fixture.callInt.__code__, type(f.__code__))

    def testJMethod_defaults(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertEqual(fixture.callInt.__defaults__, None)

    def testJMethod_kwdefaults(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertEqual(fixture.callInt.__kwdefaults__, None)

    def testJMethod_globals(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertIsInstance(fixture.callInt.__globals__, dict)

    def testJMethod_qualname(self):
        fixture = JClass("jpype.common.Fixture")()
        self.assertIsInstance(fixture.callInt.__qualname__, str)
        self.assertEqual(fixture.callInt.__qualname__,
                         'jpype.common.Fixture.callInt')

    def testMatches(self):
        js = JClass("java.lang.String")()
        self.assertFalse(js.substring._matches(object()))
        self.assertTrue(js.substring._matches(1))
        self.assertTrue(js.substring._matches(1, 2))
        self.assertFalse(js.substring._matches(1, 2, 3))
