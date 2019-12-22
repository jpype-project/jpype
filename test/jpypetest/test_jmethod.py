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
import sys
import jpype
import common
import types
import functools
import inspect

# Code from stackoverflow
# Reference http://stackoverflow.com/questions/13503079/how-to-create-a-copy-of-a-python-function
def copy_func(f):
    """Based on http://stackoverflow.com/a/6528148/190597 (Glenn Maynard)"""
    if sys.version_info[0] < 3:
        g = types.FunctionType(f.func_code, f.func_globals, name=f.func_name,
                               argdefs=f.func_defaults,
                               closure=f.func_closure)
    else:
        g = types.FunctionType(f.__code__, f.__globals__, name=f.__name__,
                               argdefs=f.__defaults__,
                               closure=f.__closure__)
        g.__kwdefaults__ = f.__kwdefaults__
    
    g = functools.update_wrapper(g, f)
    return g


@common.pytest.mark.usefixtures("common_opts")
class JMethodTestCase(common.JPypeTestCase):
    """ Test for methods of JMethod (_jpype.PyJPMethod)

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

    @common.unittest.skipIf(sys.version_info[0] < 3, "skip on Python2")
    def testMethodQualName(self):
        self.assertEqual(self.cls.substring.__qualname__, "java.lang.String.substring")
        self.assertEqual(self.obj.substring.__qualname__, "java.lang.String.substring")

    def testMethodDoc(self):
        self.assertIsInstance(self.cls.substring.__doc__, str)
        self.assertIsInstance(self.obj.substring.__doc__, str)

    def testMethodInspectDoc(self):
        self.assertIsInstance(inspect.getdoc(self.cls.substring), str)
        self.assertIsInstance(inspect.getdoc(self.obj.substring), str)

    def testMethodAnnotations(self):
        self.assertIsInstance(self.cls.substring.__annotations__, dict)
        self.assertIsNotNone(self.obj.substring.__annotations__, dict)

        # This one will need to change in Python 3.8 
        self.assertEqual(self.cls.substring.__annotations__["return"], self.cls)

    @common.unittest.skipIf(sys.version_info[0] < 3, "skip on Python2")
    def testMethodInspectSignature(self):
        self.assertIsInstance(inspect.signature(self.cls.substring), inspect.Signature)
        self.assertIsInstance(inspect.signature(self.obj.substring), inspect.Signature)
        self.assertEqual(inspect.signature(self.obj.substring).return_annotation, self.cls)

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
