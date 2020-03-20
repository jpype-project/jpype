import _jpype
import jpype
import common
from jpype.types import *


class JCharTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testCharRange(self):
        self.assertEqual(ord(str(jpype.JChar(65))), 65)
        self.assertEqual(ord(str(jpype.JChar(512))), 512)

    @common.requireInstrumentation
    def testJPChar_new(self):
        _jpype.fault("PyJPChar_new")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        _jpype.fault("PyJPModule_getContext")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar("a")
        JChar("a")

    @common.requireInstrumentation
    def testJPChar_str(self):
        jc = JChar("a")
        _jpype.fault("PyJPChar_str")
        with self.assertRaisesRegex(SystemError, "fault"):
            str(jc)
        _jpype.fault("PyJPModule_getContext")
        str(jc)

    @common.requireInstrumentation
    def testJCharGetJavaConversion(self):
        _jpype.fault("JPCharType::getJavaConversion")
        with self.assertRaisesRegex(SystemError, "fault"):
            JChar._canConvertToJava(object())

    @common.requireInstrumentation
    def testJPJavaFrameCharArray(self):
        ja = JArray(JChar)(5)
        _jpype.fault("JPJavaFrame::NewCharArray")
        with self.assertRaisesRegex(SystemError, "fault"):
            JArray(JChar)(1)
        _jpype.fault("JPJavaFrame::SetCharArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0] = 0
        _jpype.fault("JPJavaFrame::GetCharArrayRegion")
        with self.assertRaisesRegex(SystemError, "fault"):
            print(ja[0])
        _jpype.fault("JPJavaFrame::GetCharArrayElements")
        # Special case, only BufferError is allowed from getBuffer
        with self.assertRaises(BufferError):
            memoryview(ja[0:3])
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[0:3] = bytes([1, 2, 3])
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")
        with self.assertRaisesRegex(SystemError, "fault"):
            jpype.JObject(ja[::2], jpype.JObject)
        _jpype.fault("JPJavaFrame::ReleaseCharArrayElements")

        def f():
            # Special case no fault is allowed
            memoryview(ja[0:3])
        f()
        ja = JArray(JChar)(5)  # lgtm [py/similar-function]
        _jpype.fault("JPCharType::setArrayRange")
        with self.assertRaisesRegex(SystemError, "fault"):
            ja[1:3] = [0, 0]

    def testFromObject(self):
        ja = JArray(JChar)(5)
        with self.assertRaises(TypeError):
            ja[1] = object()
        jf = JClass("jpype.common.Fixture")
        with self.assertRaises(TypeError):
            jf.static_char_field = object()
        with self.assertRaises(TypeError):
            jf().char_field = object()

    def testCharArrayAsString(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getCharArray()
        self.assertEqual(str(v), 'avcd')

    def testArrayConversionChar(self):
        t = JClass("jpype.array.TestArray")()
        v = t.getCharArray()
        self.assertEqual(str(v[:]), 'avcd')

    def testArrayEqualsChar(self):
        contents = "abc"
        array = jpype.JArray(jpype.JChar)(contents)
        array2 = jpype.JArray(jpype.JChar)(contents)
        self.assertEqual(array, array)
        self.assertNotEqual(array, array2)
        self.assertEqual(array, "abc")

    def testArrayHash(self):
        ja = JArray(JByte)([1, 2, 3])
        self.assertIsInstance(hash(ja), int)

    def testNone(self):
        self.assertEqual(JChar._canConvertToJava(None), "none")
