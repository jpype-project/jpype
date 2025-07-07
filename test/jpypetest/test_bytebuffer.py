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
import _jpype
import common


class ByteBufferCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testConvertToDirectBuffer(self):
        a = bytearray([0, 0, 0, 0])
        bb = jpype.nio.convertToDirectBuffer(a)
        self.assertIsInstance(bb, jpype.JClass("java.nio.DirectByteBuffer"))
        self.assertFalse(bb.isReadOnly())
        bb.put(1)
        bb.put(2)
        bb.put(3)
        bb.put(4)
        self.assertEqual(a, bytearray([1, 2, 3, 4]))
        with self.assertRaises(jpype.JException):
            bb.put(5)

    def testConvertToReadonlyBuffer(self):
        a = bytes([1, 2, 3, 4])
        bb = jpype.nio.convertToDirectBuffer(a)
        self.assertIsInstance(bb, jpype.JClass("java.nio.DirectByteBuffer"))
        self.assertTrue(bb.isReadOnly())
        self.assertEqual(a, bytearray([1, 2, 3, 4]))
        with self.assertRaises(jpype.JException):
            bb.put(0, 1)

    def testConvertMemoryViewToReadonlyBuffer(self):
        a = bytearray([1, 2, 3, 4])
        a = memoryview(a).toreadonly()
        bb = jpype.nio.convertToDirectBuffer(a)
        self.assertIsInstance(bb, jpype.JClass("java.nio.DirectByteBuffer"))
        self.assertTrue(bb.isReadOnly())
        self.assertEqual(a, bytearray([1, 2, 3, 4]))
        with self.assertRaises(jpype.JException):
            bb.put(0, 1)

    def testRepr(self):
        a = bytearray([0, 0, 0, 0])
        bb = jpype.nio.convertToDirectBuffer(a)
        self.assertIsInstance(repr(bb), str)
        self.assertEqual(repr(bb), "<java buffer 'java.nio.DirectByteBuffer'>")

    def testMemoryView(self):
        self.assertEqual(memoryview(jpype.java.nio.ByteBuffer.allocateDirect(100)).nbytes, 100)
