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
from jpype import JPackage, java
import common
import pytest
try:
    import numpy as np
except ImportError:
    pass


class ArrayTestCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.VALUES = [12234, 1234, 234, 1324, 424, 234, 234, 142, 5, 251, 242, 35, 235, 62,
                       1235, 46, 245132, 51, 2, 3, 4]

    @common.requireInstrumentation
    def testJPArray_init(self):
        _jpype.fault("PyJPArray_init")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JInt)(5)

    def testJPArray_initExc(self):
        with self.assertRaises(TypeError):
            _jpype._JArray("foo")
        with self.assertRaises(TypeError):
            JArray(JInt)(JArray(JDouble)([1, 2]))
        with self.assertRaises(TypeError):
            JArray(JInt)(JString)
        with self.assertRaises(ValueError):
            JArray(JInt)(-1)
        with self.assertRaises(ValueError):
            JArray(JInt)(10000000000)
        with self.assertRaises(TypeError):
            JArray(JInt)(object())
        self.assertEqual(len(JArray(JInt)(0)), 0)
        self.assertEqual(len(JArray(JInt)(10)), 10)
        self.assertEqual(len(JArray(JInt)([1, 2, 3])), 3)
        self.assertEqual(len(JArray(JInt)(JArray(JInt)([1, 2, 3]))), 3)

        class badlist(list):
            def __len__(self):
                return -1
        with self.assertRaises(ValueError):
            JArray(JInt)(badlist([1, 2, 3]))

    @common.requireInstrumentation
    def testJPArray_repr(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_repr")
        with self.assertRaisesRegex(SystemError, "fault"):
            repr(ja)

    @common.requireInstrumentation
    def testJPArray_len(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_len")
        with self.assertRaisesRegex(SystemError, "fault"):
            len(ja)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            len(ja)

    @common.requireInstrumentation
    def testJPArray_getArrayItem(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_getArrayItem")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])

    def testJPArray_getArrayItemExc(self):
        ja = JArray(JInt)(5)
        with self.assertRaises(TypeError):
            ja[object()]
        with self.assertRaises(ValueError):
            ja[slice(0, 0, 0)]
        self.assertEqual(len(JArray(JInt)(5)[4:1]), 0)

    @common.requireInstrumentation
    def testJPArray_assignSubscript(self):
        ja = JArray(JInt)(5)
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:2] = 1
        _jpype.fault("PyJPArray_assignSubscript")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 1
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:2] = 1

    def testJPArray_assignSubscriptExc(self):
        ja = JArray(JInt)(5)
        with self.assertRaises(ValueError):
            del ja[0:2]
        with self.assertRaises(ValueError):
            ja[slice(0, 0, 0)] = 1

    @common.requireInstrumentation
    def testJPArray_getBuffer(self):
        _jpype.fault("PyJPArray_getBuffer")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja = JArray(JInt, 2)(5)
            m = memoryview(ja)
            del m  # lgtm [py/unnecessary-delete]

    @common.requireInstrumentation
    def testJPArrayPrimitive_getBuffer(self):
        _jpype.fault("PyJPArrayPrimitive_getBuffer")

        def f():
            ja = JArray(JInt)(5)
            m = memoryview(ja)
            del m  # lgtm [py/unnecessary-delete]
        with self.assertRaisesRegex(SystemError, "fault"):
            f()
        with self.assertRaises(BufferError):
            memoryview(JArray(JInt, 2)([[1, 2], [1], [1, 2, 3]]))
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            f()

    @common.requireInstrumentation
    def testJPArray_null(self):
        _jpype.fault("PyJPArray_init.null")
        null = JArray(JInt)(object())
        with self.assertRaises(ValueError):
            len(null)
        with self.assertRaises(ValueError):
            null[0]
        with self.assertRaises(ValueError):
            null[0] = 1
        with self.assertRaises(ValueError):
            memoryview(null)
        null = JArray(JObject)(object())
        with self.assertRaises(ValueError):
            memoryview(null)
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            memoryview(null)
        with self.assertRaisesRegex(TypeError, 'Not a Java value'):
            _jpype._JObject.__str__(null)
        self.assertEqual(hash(null), hash(None))

    @common.requireInstrumentation
    def testJPArrayNew(self):
        ja = JArray(JInt)
        _jpype.fault("JPArray::JPArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja(5)
        j = ja(5)
        _jpype.fault("JPArray::JPArraySlice")
        with self.assertRaisesRegex(SystemError, "fault"):
            j[0:2:1]

    @common.requireInstrumentation
    def testJArrayClassConvertToVector(self):
        Path = JClass("java.nio.file.Paths")
        _jpype.fault("JPArrayClass::convertToJavaVector")
        with self.assertRaisesRegex(SystemError, "fault"):
            Path.get("foo", "bar")

    @common.requireInstrumentation
    def testJArrayGetJavaConversion(self):
        ja = JArray(JInt)
        _jpype.fault("JPArrayClass::findJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja._canConvertToJava(object())

    @common.requireInstrumentation
    def testJArrayConvertToPythonObject(self):
        jl = JClass('java.util.ArrayList')()
        jl.add(JArray(JInt)(3))
        _jpype.fault("JPArrayClass::convertToPythonObject")
        with self.assertRaisesRegex(SystemError, "fault"):
            jl.get(0)

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

    @common.requireNumpy
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
            a[1:3] = b[1:4]

    def testJArrayIndexOutOfBounds(self):
        a = JArray(JObject)(10)
        with self.assertRaises(IndexError):
            a[50] = 1
        with self.assertRaises(IndexError):
            a[-50] = 1

    def testJArrayBadItem(self):
        a = JArray(JObject)(10)

        class Larry(object):
            pass
        with self.assertRaises(TypeError):
            a[1] = Larry()

    def testJArrayCopyRange(self):
        a = JArray(JObject)(['a', 'b', 'c', 'd', 'e', 'f'])
        a[1:4] = ['x', 'y', 'z']
        self.assertEqual(list(a), ['a', 'x', 'y', 'z', 'e', 'f'])

    def checkArrayOf(self, jtype, dtype, mn=None, mx=None):
        if mn and mx:
            a = np.random.randint(mn, mx, size=100, dtype=dtype)
        else:
            a = np.random.random(100).astype(dtype)
        ja = JArray.of(a)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertEqual(len(ja), 100)
        self.assertTrue(np.all(a == ja))

        a = np.reshape(a, (20, 5))
        ja = JArray.of(a)
        self.assertIsInstance(ja, JArray(jtype, 2))
        self.assertEqual(len(ja), 20)
        self.assertEqual(len(ja[0]), 5)
        self.assertTrue(np.all(a == ja))

        a = np.reshape(a, (5, 2, 10))
        ja = JArray.of(a)
        self.assertIsInstance(ja, JArray(jtype, 3))
        self.assertEqual(len(ja), 5)
        self.assertEqual(len(ja[0]), 2)
        self.assertEqual(len(ja[0][0]), 10)
        self.assertTrue(np.all(a == ja))
        self.assertTrue(np.all(a[1, :, :] == JArray.of(a[1, :, :])))
        self.assertTrue(np.all(a[2::2, :, :] == JArray.of(a[2::2, :, :])))
        self.assertTrue(np.all(a[:, :, 4:-2] == JArray.of(a[:, :, 4:-2])))

    def checkArrayOfCast(self, jtype, dtype):
        a = np.random.randint(0, 1, size=100, dtype=np.bool)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(0, 2 * 8 - 1, size=100, dtype=np.uint8)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(-2 * 7, 2 * 7 - 1, size=100, dtype=np.int8)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(0, 2 * 16 - 1, size=100, dtype=np.uint16)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(-2 * 15, 2 * 15 - 1, size=100, dtype=np.int16)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(0, 2 * 32 - 1, size=100, dtype=np.uint32)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(-2 * 31, 2 * 31 - 1, size=100, dtype=np.int32)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(0, 2 * 64 - 1, size=100, dtype=np.uint64)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

        a = np.random.randint(-2 * 63, 2 * 63 - 1, size=100, dtype=np.int64)
        ja = JArray.of(a, dtype=jtype)
        self.assertIsInstance(ja, JArray(jtype))
        self.assertTrue(np.all(a.astype(dtype) == ja))

    @common.requireNumpy
    def testArrayOfBoolean(self):
        self.checkArrayOf(JBoolean, np.bool, 0, 1)

    @common.requireNumpy
    def testArrayOfByte(self):
        self.checkArrayOf(JByte, np.int8, -2**7, 2**7 - 1)

    @common.requireNumpy
    def testArrayOfShort(self):
        self.checkArrayOf(JShort, np.int16, -2**15, 2**15 - 1)

    @common.requireNumpy
    def testArrayOfInt(self):
        self.checkArrayOf(JInt, np.int32, -2**31, 2**31 - 1)

    @common.requireNumpy
    def testArrayOfLong(self):
        self.checkArrayOf(JLong, np.int64, -2**63, 2**63 - 1)

    @common.requireNumpy
    def testArrayOfFloat(self):
        self.checkArrayOf(JFloat, np.float32)

    @common.requireNumpy
    def testArrayOfDouble(self):
        self.checkArrayOf(JDouble, np.float64)

    @common.requireNumpy
    def testArrayOfBooleanCast(self):
        self.checkArrayOfCast(JBoolean, np.bool)

    @common.requireNumpy
    def testArrayOfByteCast(self):
        self.checkArrayOfCast(JByte, np.int8)

    @common.requireNumpy
    def testArrayOfShortCast(self):
        self.checkArrayOfCast(JShort, np.int16)

    @common.requireNumpy
    def testArrayOfIntCast(self):
        self.checkArrayOfCast(JInt, np.int32)

    @common.requireNumpy
    def testArrayOfLongCast(self):
        self.checkArrayOfCast(JLong, np.int64)

    @common.requireNumpy
    def testArrayOfFloatCast(self):
        self.checkArrayOfCast(JFloat, np.float32)

    @common.requireNumpy
    def testArrayOfDoubleCast(self):
        self.checkArrayOfCast(JDouble, np.float64)

    @common.requireNumpy
    def testArrayOfExc(self):
        a = np.random.randint(0, 2 * 8 - 1, size=1, dtype=np.uint8)
        with self.assertRaises(TypeError):
            JArray.of(a)
        a = np.random.randint(0, 2 * 16 - 1, size=1, dtype=np.uint16)
        with self.assertRaises(TypeError):
            JArray.of(a)
        a = np.random.randint(0, 2 * 32 - 1, size=1, dtype=np.uint32)
        with self.assertRaises(TypeError):
            JArray.of(a)
        a = np.random.randint(0, 2 * 64 - 1, size=1, dtype=np.uint64)
        with self.assertRaises(TypeError):
            JArray.of(a)

    @common.requireInstrumentation
    def testArrayOfFaults(self):
        b = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::assemble")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JInt)
        _jpype.fault("JPBooleanType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JBoolean)
        _jpype.fault("JPCharType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JChar)
        _jpype.fault("JPByteType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JByte)
        _jpype.fault("JPShortType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JShort)
        _jpype.fault("JPIntType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JInt)
        _jpype.fault("JPLongType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JLong)
        _jpype.fault("JPFloatType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JFloat)
        _jpype.fault("JPDoubleType::newMultiArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray.of(b, JDouble)

    def testJArrayDimTooBig(self):
        with self.assertRaises(ValueError):
            jpype.JArray(jpype.JInt, 10000)

    def testJArrayDimWrong(self):
        with self.assertRaises(TypeError):
            jpype.JArray(jpype.JInt, 1.5)

    def testJArrayArgs(self):
        with self.assertRaises(TypeError):
            jpype.JArray(jpype.JInt, 1, 'f')

    def testJArrayTypeBad(self):
        class John(object):
            pass
        with self.assertRaises(TypeError):
            jpype.JArray(John)

    @common.requireNumpy
    def testZeroArray(self):
        ls = []
        ja = JArray(JInt)(ls)
        self.assertEqual(len(ja), 0)
        na = np.array(ja)
        self.assertEqual(len(ja), 0)
        ja = JArray(JString)(ls)
        with self.assertRaisesRegex(BufferError, "not primitive array"):
            memoryview(ja)
        self.assertEqual(len(ja), 0)
        na = np.array(ja)
        self.assertEqual(len(ja), 0)
        ja = JArray(JInt, 2)([[], [1]])
        with self.assertRaisesRegex(BufferError, "not rectangular"):
            memoryview(ja)
        ja = JArray(JInt, 2)([[1], []])
        with self.assertRaisesRegex(BufferError, "not rectangular"):
            memoryview(ja)

    def testLengthProperty(self):
        ja = JArray(JInt)([1, 2, 3])
        self.assertEqual(ja.length, len(ja))
