import jpype
import _jpype
import common


class ByteBufferCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testConvertToDirectBuffer(self):
        a = bytearray([0, 0, 0, 0])
        bb = jpype.nio.convertToDirectBuffer(a)
        self.assertIsInstance(bb, jpype.JClass("java.nio.DirectByteBuffer"))
        bb.put(1)
        bb.put(2)
        bb.put(3)
        bb.put(4)
        self.assertEqual(a, bytearray([1, 2, 3, 4]))
        with self.assertRaises(jpype.JException):
            bb.put(5)

    def testConvertToDirectBufferFail(self):
        a = bytes([0, 0, 0, 0])
        with self.assertRaises(ValueError):
            bb = jpype.nio.convertToDirectBuffer(a)
