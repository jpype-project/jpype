import sys
import jpype
from jpype.types import *
from jpype import JPackage, java
import common
import pytest

def haveNumpy():
    try:
        import numpy
        return True
    except ImportError:
        return False


class ArrayTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.VALUES = [12234, 1234, 234, 1324, 424, 234, 234, 142, 5, 251, 242, 35, 235, 62,
                       1235, 46, 245132, 51, 2, 3, 4]

    def testReadArray(self):
        t = JClass("jpype.array.TestArray")()
        self.assertNotIsInstance(t, JPackage)

        self.assertCountEqual(self.VALUES, t.i)

        self.assertEqual(t.i[0], self.VALUES[0])
        self.assertCountEqual(self.VALUES[1:-2], t.i[1:-2])

    def testEmptyObjectArray(self):
        ''' Test for strange crash reported in bug #1089302'''
        Test2 = jpype.JPackage('jpype.array').Test2
        test = Test2()
        test.test(test.getValue())

    def testWriteArray(self):
        t = JClass("jpype.array.TestArray")()
        self.assertNotIsInstance(t, JPackage)

        t.i[0] = 32
        self.assertEqual(t.i[0], 32)

        t.i[1:3] = (33, 34)
        self.assertEqual(t.i[1], 33)
        self.assertEqual(t.i[2], 34)

        self.assertCountEqual(t.i[:5], (32, 33, 34, 1324, 424))

    def testObjectArraySimple(self):
        a = JArray(java.lang.String, 1)(2)
        a[1] = "Foo"
        self.assertEqual("Foo", a[1])

    def testIterateArray(self):
        t = JClass("jpype.array.TestArray")()
        self.assertFalse(isinstance(t, JPackage))

        for i in t.i:
            self.assertNotEqual(i, 0)

    def testGetSubclass(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getSubClassArray()
        self.assertTrue(isinstance(v[0], jpype.java.lang.Integer))

    def testGetArrayAsObject(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getArrayAsObject()

    def testJArrayPythonTypes(self):
        self.assertEqual(jpype.JArray(
            object).class_.getComponentType(), JClass('java.lang.Object'))
        self.assertEqual(jpype.JArray(
            float).class_.getComponentType(), JClass('java.lang.Double').TYPE)
        self.assertEqual(jpype.JArray(
            str).class_.getComponentType(), JClass('java.lang.String'))
        self.assertEqual(jpype.JArray(
            type).class_.getComponentType(), JClass('java.lang.Class'))

    def testObjectArrayInitial(self):
        l1 = java.util.ArrayList()
        l1.add(0)
        l2 = java.util.ArrayList()
        l2.add(42)
        l3 = java.util.ArrayList()
        l3.add(13)
        jarr = jpype.JArray(java.util.ArrayList, 1)([l1, l2, l3])

        self.assertEqual(l1, jarr[0])
        self.assertEqual(l2, jarr[1])
        self.assertEqual(l3, jarr[2])

    @common.unittest.skipUnless(haveNumpy(), "numpy not available")
    def testSetFromNPByteArray(self):
        import numpy as np
        n = 100
        a = np.random.randint(-128, 127, size=n).astype(np.byte)
        jarr = jpype.JArray(jpype.JByte)(n)
        jarr[:] = a
        self.assertCountEqual(a, jarr)

    def testArrayCtor1(self):
        jobject = jpype.JClass('java.lang.Object')
        jarray = jpype.JArray(jobject)
        self.assertTrue(issubclass(jarray, jpype.JArray))
        self.assertTrue(isinstance(jarray(10), jpype.JArray))

    def testArrayCtor2(self):
        jobject = jpype.JClass('java.util.List')
        jarray = jpype.JArray(jobject)
        self.assertTrue(issubclass(jarray, jpype.JArray))
        self.assertTrue(isinstance(jarray(10), jpype.JArray))

    def testArrayCtor3(self):
        jarray = jpype.JArray("java.lang.Object")
        self.assertTrue(issubclass(jarray, jpype.JArray))
        self.assertTrue(isinstance(jarray(10), jpype.JArray))

    def testArrayCtor4(self):
        jarray = jpype.JArray(jpype.JObject)
        self.assertTrue(issubclass(jarray, jpype.JArray))
        self.assertTrue(isinstance(jarray(10), jpype.JArray))

    def testArrayCtor5(self):
        jarray0 = jpype.JArray("java.lang.Object")
        jarray = jpype.JArray(jarray0)
        self.assertTrue(issubclass(jarray, jpype.JArray))
        self.assertTrue(isinstance(jarray(10), jpype.JArray))

    def testObjectNullArraySlice(self):
        # Check for bug in 0.7.0
        array = jpype.JArray(jpype.JObject)([None, ])
        self.assertEqual(tuple(array[:]), (None,))

    def testJArrayBadType(self):
        class Bob(object):
            pass
        with self.assertRaises(TypeError):
            jpype.JArray(Bob)

    def testJArrayBadSlice(self):
        ja = JArray(JObject)(5)
        with self.assertRaises(TypeError):
            ja[dict()] = 1

    def testJArrayBadAssignment(self):
        ja = JArray(JObject)(5)
        with self.assertRaises(TypeError):
            ja[1:3] = dict()

    def testJArrayIncorrectSliceLen(self):
        a = JArray(JObject)(10)
        b = JArray(JObject)(10)
        with self.assertRaises(ValueError):
            a[1:3]=b[1:4]

    def testJArrayIndexOutOfBounds(self):
        a = JArray(JObject)(10)
        with self.assertRaises(IndexError):
            a[50]=1
        with self.assertRaises(IndexError):
            a[-50]=1

    def testJArrayBadItem(self):
        a = JArray(JObject)(10)
        class Larry(object):
                pass
        with self.assertRaises(TypeError):
            a[1]=Larry()

    def testJArrayCopyRange(self):
        a = JArray(JObject)(['a','b','c','d','e','f'])
        a[1:4]=['x','y','z']
        self.assertEqual(list(a), ['a','x','y','z','e','f'])

