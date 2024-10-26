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
from __future__ import annotations
import common
import gc
from jpype._jclass import *
from jpype.types import *
from jpype.imports import *
from jpype import JParameterAnnotation
import importlib.abc
import importlib.util
import textwrap
import sys


class JExtensionTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testExtendObject(self):
        from java.lang import Object
        class MyObject(Object):

            @JPublic
            def __init__(self):
                ...

        self.assertIsInstance(MyObject(), MyObject)

    def testClassAttribute(self):
        from java.lang import Class, Object
        class MyObject(Object):

            @JPublic
            def __init__(self):
                ...

        self.assertIsInstance(MyObject.class_, Class)
        self.assertIs(MyObject, JClass(MyObject))

    def testSameObject(self):
        from java.lang import Object
        class MyObject(Object):

            @JPublic
            def __init__(self):
                ...

            def get_self(self) -> Object:
                return self

        obj = MyObject()
        self.assertIs(obj, obj.get_self())

    def testAddedMethod(self):
        from java.lang import Object
        class MyObject(Object):

            @JPublic
            def __init__(self):
                ...

            @JPublic
            def func(self) -> JObject:
                return self

        o = MyObject()
        self.assertIs(o.func(), o)

    def testInitOnce(self):
        TestBase = JClass("jpype.extension.TestBase")
        class MyObject(TestBase):

            def __init__(self):
                super().__init__()
                self.initCount += 1

            @JPublic
            def __init__(self):
                self.initCount += 1

            @JPublic
            def get_self(self) -> JObject:
                return self

        o = MyObject().get_self()
        # once in TestBase constructor, then in the __init__ callback from constructor
        # then again when the Python __init__ is called
        self.assertEqual(o.initCount, 3)

    def testOverrideSimple(self):
        from java.lang import Object, String
        class MyObject(Object):

            @JPublic
            def __init__(self):
                ...

            @JPublic
            @JOverride
            def toString(self) -> String:
                return "test"

        self.assertEqual(str(MyObject()), "test")

    def testOverloads(self):
        mode = -1

        class MyObject(JClass("jpype.extension.TestBase")):

            def __init__(self, *args, **kwargs):
                pass

            @JPublic
            def __init__(self):
                nonlocal mode
                mode = 0

            @JPublic
            def __init__(self, i: JInt):
                nonlocal mode
                mode = 1

            @JPublic
            def __init__(self, o: JObject):
                nonlocal mode
                mode = 2

            @JPublic
            @JOverride
            def identity(self, i: JInt) -> JInt:
                return 0

            @JPublic
            @JOverride
            def identity(self, o: JObject) -> JObject:
                return None

            @JPublic
            def func(self) -> JObject:
                return self

        o = MyObject()
        self.assertEqual(mode, 0)
        MyObject(JInt(1))
        self.assertEqual(mode, 1)
        MyObject(JObject())
        self.assertEqual(mode, 2)
        self.assertEqual(o.identity(JInt(1)), 0)
        self.assertEqual(o.identity(JObject()), None)

    def testSupercall(self):
        TestBase = JClass("jpype.extension.TestBase")
        class MyObject(TestBase):

            @JPublic
            def __init__(self):
                pass

            @JPublic
            @JOverride
            def identity(self, o: JObject) -> JObject:
                return None # type: ignore[return-value]

            @JPublic
            def super_identity(self, o: JObject) -> JObject:
                return super().identity(o)

            def test_identity(self, o: JObject) -> JObject:
                return super().identity(o)

            def test_identity_explicit(self, o: JObject) -> JObject:
                return super(TestBase, self).identity(o)

            def get_super(self):
                return super()

            def get_explicit_super(self):
                return super(TestBase, self)

        o = MyObject()
        sentinel = MyObject()
        self.assertIs(o.test_identity(sentinel), sentinel)
        self.assertIs(o.super_identity(sentinel), sentinel)


    def testPythonMembers(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            def __init__(self):
                self.a = 0
                self.b = 1

            @JPublic
            def __init__(self):
                ...

            @JPublic
            def get_self(self) -> JObject:
                return self

        o = MyObject()
        o2 = o.get_self()
        self.assertIs(o, o2)
        self.assertEqual(o.a, 0)
        self.assertEqual(o.b, 1)


    def testConstructFromJava(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            def __init__(self):
                self.a = 0
                self.b = 1

            @JPublic
            def __init__(self):
                # caveat Python __init__ isn't called when constructed from Java
                self.__init__()

            @JPublic
            def get_self(self) -> JObject:
                return self

        o = MyObject.class_.newInstance()
        o2 = o.get_self()
        self.assertIs(o, o2)
        self.assertEqual(o.a, 0)
        self.assertEqual(o.b, 1)

    def testProtectedField(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            def get_protected_field(self):
                return self.protectedField

        o = MyObject()
        o.get_protected_field()

    def testProtectedFieldExternalAccess(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

        o = MyObject()
        with self.assertRaises(AttributeError):
            o.protectedField

    def testPrivateBaseField(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            def get_private_field(self):
                return self.privateBaseField

        o = MyObject()
        with self.assertRaises(AttributeError):
            o.get_private_field()

    def testPublicField(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            myField: JPublic[JInt]

            def __init__(self):
                self.pythonMember = None

            @JPublic
            def __init__(self):
                # java exposed constructor, called before python __init__
                ...

        o = MyObject()
        o.myField

    def testPublicFieldWithValue(self):
        from java.lang import IllegalArgumentException
        with self.assertRaises(IllegalArgumentException):
            class MyObject(JClass("jpype.extension.TestBase")):
                test: JPublic[JInt] = JInt(1)

                @JPublic
                def __init__(self):
                    ...

    def testPublicFinalField(self):
        unittest = self
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JFinal[JPublic[JInt]]

            @JPublic
            def __init__(self):
                self.test = 1
                unittest.assertEqual(self.test, 1)
                self.test = 2
                unittest.assertEqual(self.test, 2)

        o = MyObject()
        with self.assertRaises(AttributeError):
            o.test = 3


    def testPrivateField(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JPrivate[JInt]

            @JPublic
            def __init__(self):
                ...

            def get_private_field(self):
                return self.test

        o = MyObject()
        o.get_private_field()

    def testPrivateFieldExternalAccess(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JPrivate[JInt]

            @JPublic
            def __init__(self):
                ...

            def get_private_field(self):
                return self.test

        o = MyObject()
        with self.assertRaises(AttributeError):
            o.test

    def testPublicStaticField(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JStatic[JPublic[JInt]]

            @JPublic
            def __init__(self):
                ...

        o = MyObject()
        o.test

    def testPublicStaticFieldWithValue(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JStatic[JPublic[JInt]] = JInt(1)

            @JPublic
            def __init__(self):
                ...

        o = MyObject()
        self.assertEqual(o.test, 1)
        o.test = 2
        self.assertEqual(o.test, 2)

    def testPublicStaticFinalFieldWithValue(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JFinal[JStatic[JPublic[JInt]]] = JInt(1)

            @JPublic
            def __init__(self):
                ...

        o = MyObject()
        self.assertEqual(o.test, 1)
        with self.assertRaises(AttributeError):
            o.test = 2

    def testPrivateStaticField(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JStatic[JPrivate[JInt]]

            @JPublic
            def __init__(self):
                ...

            def get_private_field(self):
                return self.test

        o = MyObject()
        o.get_private_field()

    def testPrivateStaticFieldExternalAccess(self):
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JStatic[JPrivate[JInt]]

            @JPublic
            def __init__(self):
                ...

            def get_private_field(self):
                return self.test

        o = MyObject()
        with self.assertRaises(AttributeError):
            o.test

    def testPrivateMethod(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            def private_method(self) -> JObject:
                return self

            def call_private_method(self):
                return self.private_method()

        o = MyObject()
        o.call_private_method()

    def testPrivateMethodExternalAccess(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            def private_method(self) -> JObject:
                return self

        o = MyObject()
        with self.assertRaises(AttributeError):
            o.private_method()

    def testPrivateStaticMethod(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JInt):
                self.assertIs(cls, MyObject)

            def call_private_method(self):
                return self.private_method(0)

        o = MyObject()
        o.call_private_method()

    def testPrivateStaticMethodFromClassmethod(self):
        # FIXME: bytecode verification error
        # forgot to handle static methods in class generation
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @classmethod
            def private_method(cls, v: JInt):
                self.assertIs(cls, MyObject)

            def call_private_method(self):
                return self.private_method(0)

        o = MyObject()
        o.call_private_method()

    def testFinalMethod(self):
        from java.lang import IncompatibleClassChangeError, Object, String
        class MyBaseObject(Object):

            @JPublic
            def __init__(self):
                ...

            @JPublic
            @JFinal
            def toString(self) -> String:
                return "test"

        # if this is raised then it means the method was marked as final
        with self.assertRaises(IncompatibleClassChangeError):
            class MyObject(MyBaseObject):

                @JPublic
                @JOverride
                def toString(self) -> String:
                    return "fail"

    def testThrows(self):
        from java.lang import Object, UnsupportedOperationException, IllegalArgumentException
        from java.lang import IllegalCallerException, Throwable, RuntimeException
        class MyObject(Object):

            @JPublic
            @JThrows(UnsupportedOperationException, IllegalArgumentException)
            @JThrows(IllegalCallerException)
            @JThrows(Throwable, RuntimeException)
            def __init__(self):
                ...

        ctor = MyObject.class_.getDeclaredConstructors()[0]
        exceptions = (
            UnsupportedOperationException, IllegalArgumentException,
            IllegalCallerException,
            Throwable, RuntimeException
        )
        self.assertEqual(tuple(ctor.getExceptionTypes()), exceptions)


    def testThrownException(self):
        from java.lang import Object, String, Throwable
        class MyObject(Object):

            @JPublic
            def __init__(self):
                ...

            @JPublic
            def toString(self) -> String:
                raise Throwable("fdnvdnkvnrne")

        o = MyObject()
        with self.assertRaises(Throwable):
            str(o)


    def testExtensionLoaderMutipleClassDefinition(self):
        class TestLoader(importlib.abc.InspectLoader):

            def get_source(self, _):
                return textwrap.dedent(
                    """\
                    from jpype import JPublic
                    from java.lang import Object
                    class TemporaryObject(Object):

                        @JPublic
                        def __init__(self):
                            ...

                    """
                )

        def create_class():
            loader = TestLoader()
            spec = importlib.util.spec_from_loader("__main__", loader)
            module = importlib.util.module_from_spec(spec)
            loader.exec_module(module)

        create_class()
        create_class()

    def testExtensionLoaderCleanup(self):
        JPypeContext = JClass("org.jpype.JPypeContext")
        manager = JPypeContext.getInstance().getTypeManager()

        class TestLoader(importlib.abc.InspectLoader):

            def get_source(self, _):
                return textwrap.dedent(
                    """\
                    from jpype import JPublic
                    from java.lang import Object
                    class TemporaryObject(Object):

                        @JPublic
                        def __init__(self):
                            ...

                    """
                )

        loader = TestLoader()

        # NOTE: module instances are never collected when debugging
        spec = importlib.util.spec_from_loader("__main__", loader)
        m = importlib.util.module_from_spec(spec)
        start = manager.classMap.size()
        loader.exec_module(m)
        ldr = m.TemporaryObject.class_.getClassLoader()
        end = manager.classMap.size()
        self.assertGreater(end, start)

        # ensure it can be collected
        m.__dict__.clear()
        del spec
        del loader

        # collect it
        gc.collect()

        # check it was collected
        self.assertLess(manager.classMap.size(), end)

        ldr.closeForTesting()

    def testExtensionCleanupWithDanglingPythonReference(self):
        JPypeContext = JClass("org.jpype.JPypeContext")
        manager = JPypeContext.getInstance().getTypeManager()

        class TestLoader(importlib.abc.InspectLoader):

            def get_source(self, _):
                return textwrap.dedent(
                    """\
                    from jpype import JPublic
                    from java.lang import Object, String
                    class TemporaryObject(Object):

                        @JPublic
                        def __init__(self):
                            ...

                        @JPublic
                        def toString(self) -> String:
                            from java.lang import String
                            return String("wtf")

                        def __repr__(self):
                            from java.lang import String
                            return String.format("%s", self)

                    """
                )

        loader = TestLoader()

        # NOTE: module instances are never collected when debugging
        spec = importlib.util.spec_from_loader("__main__", loader)
        m = importlib.util.module_from_spec(spec)
        start = manager.classMap.size()
        loader.exec_module(m)
        TemporaryObject = getattr(m, "TemporaryObject")
        cls = TemporaryObject.class_
        ldr = cls.getClassLoader()
        obj = TemporaryObject()
        end = manager.classMap.size()
        self.assertGreater(end, start)

        # ensure the loader can be collected
        m.__dict__.clear()
        del spec
        del loader
        del m

        # collect it
        gc.collect()

        # check it was collected
        self.assertLess(manager.classMap.size(), end)

        with self.assertRaises(TypeError):
            TemporaryObject()

        from java.lang import InstantiationException
        with self.assertRaises(InstantiationException):
            cls.newInstance()

        #with self.assertRaises(TypeError):
        #    str(obj)
        # it needs to be something Java can catch
        from java.lang import IllegalStateException
        with self.assertRaises(IllegalStateException):
            str(obj)

        del obj
        del TemporaryObject
        del cls
        gc.collect()

        ldr.closeForTesting()


    def testExtensionCleanupWithDanglingJavaReference(self):
        JPypeContext = JClass("org.jpype.JPypeContext")
        manager = JPypeContext.getInstance().getTypeManager()

        class TestLoader(importlib.abc.InspectLoader):

            def get_source(self, _):
                return textwrap.dedent(
                    """\
                    from jpype import JPublic
                    from java.lang import Object, String
                    class TemporaryObject(Object):

                        @JPublic
                        def __init__(self):
                            ...

                        @JPublic
                        def toString(self) -> String:
                            ...

                    """
                )

        loader = TestLoader()

        # NOTE: module instances are never collected when debugging
        spec = importlib.util.spec_from_loader("__main__", loader)
        m = importlib.util.module_from_spec(spec)
        TestDanglingObject = JClass("jpype.extension.TestDanglingObject")
        start = manager.classMap.size()
        loader.exec_module(m)
        TemporaryObject = getattr(m, "TemporaryObject")
        test = TestDanglingObject(TemporaryObject())
        end = manager.classMap.size()
        self.assertGreater(end, start)

        # ensure the loader can be collected
        m.__dict__.clear()
        del spec
        del loader

        # collect it
        gc.collect()

        # check it was collected
        self.assertLess(manager.classMap.size(), end)

        from java.lang import IllegalStateException
        with self.assertRaises(IllegalStateException):
            test.test()


    def testExtensionFromBuiltinLoader(self):
        class TestLoader(importlib.abc.InspectLoader):

            def get_source(self, _):
                return textwrap.dedent(
                    """\
                    from jpype import JPublic
                    from java.lang import Object
                    class TemporaryObject(Object):

                        @JPublic
                        def __init__(self):
                            ...

                    """
                )

        loader = TestLoader()
        spec = importlib.util.spec_from_loader("dummy", loader)
        m = importlib.util.module_from_spec(spec)
        sys.modules["dummy"] = m
        loader.exec_module(m)
        sys.modules.pop("dummy")

    def testExtensionFromBuiltinLoaderReloaded(self):
        class TestLoader(importlib.abc.InspectLoader):

            def get_source(self, _):
                return textwrap.dedent(
                    """\
                    from jpype import JPublic
                    from java.lang import Object
                    class TemporaryObject(Object):

                        @JPublic
                        def __init__(self):
                            ...

                    """
                )

        loader = TestLoader()
        spec = importlib.util.spec_from_loader("dummy", loader)
        m = importlib.util.module_from_spec(spec)
        sys.modules["dummy"] = m
        loader.exec_module(m)
        # simulating importlib.reload
        loader.exec_module(m)
        sys.modules.pop("dummy")

    def testPublicFieldWithAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        class MyObject(JClass("jpype.extension.TestBase")):
            test: JPublic[JInt] = TestSimpleAnnotation(JInt(4))

            @JPublic
            def __init__(self):
                ...

        field = MyObject.class_.getDeclaredFields()[2]
        annotation = field.getAnnotation(TestSimpleAnnotation)
        self.assertIsNotNone(annotation)
        self.assertEqual(annotation.value(), 4)

    def testPublicConstructorWithAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            @TestSimpleAnnotation(JInt(4))
            def __init__(self):
                ...

        ctor = MyObject.class_.getDeclaredConstructors()[0]
        annotation = ctor.getAnnotation(TestSimpleAnnotation)
        self.assertIsNotNone(annotation)
        self.assertEqual(annotation.value(), 4)


    def testPublicMethodWithAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPublic
            @TestSimpleAnnotation(JInt(4))
            def fun(self):
                ...

        method = MyObject.class_.getDeclaredMethods()[0]
        annotation = method.getAnnotation(TestSimpleAnnotation)
        self.assertIsNotNone(annotation)
        self.assertEqual(annotation.value(), 4)

    def testPublicMethodWithParameterAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPublic
            @JParameterAnnotation("annotation_value", TestSimpleAnnotation(JInt(4)))
            def fun(self, annotation_value: JLong):
                ...

        method = MyObject.class_.getDeclaredMethods()[0]
        annotation = method.getParameterAnnotations()[0][0]
        self.assertIsNotNone(annotation)
        self.assertEqual(annotation.value(), 4)

    def testPublicClassWithAnnotation(self):
        TestSimpleAnnotation = JClass("jpype.annotation.TestSimpleAnnotation")
        class MyObject(JClass("jpype.extension.TestBase")):

            __jannotations__ = TestSimpleAnnotation(JInt(4))

            @JPublic
            def __init__(self):
                ...

        annotation = MyObject.class_.getAnnotation(TestSimpleAnnotation)
        self.assertIsNotNone(annotation)
        self.assertEqual(annotation.value(), 4)

    def testPrimitiveParameterBool(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JBoolean):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterByte(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JByte):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterChar(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JChar):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterShort(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JShort):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterInt(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JInt):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterLong(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JLong):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterFloat(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JFloat):
                self.assertIs(cls, MyObject)

    def testPrimitiveParameterDouble(self):
        class MyObject(JClass("jpype.extension.TestBase")):

            @JPublic
            def __init__(self):
                ...

            @JPrivate
            @JStatic
            def private_method(cls, v: JDouble):
                self.assertIs(cls, MyObject)

