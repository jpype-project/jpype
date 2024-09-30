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
import os
import pickle
import sys

import common
import jpype
from jpype import java
from jpype.pickle import JPickler, JUnpickler


def dump(fname):
    with open(fname, "rb") as fd:
        data = fd.read()
    out = ["%02x" % i for i in data]
    print("Pickle fail", " ".join(out), file=sys.stderr)


class PickleTestCase(common.JPypeTestCase):
    def setUp(self):
        super(PickleTestCase, self).setUp()
        self.filename = "test.pic"

    def tearDown(self):
        try:
            os.unlink(self.filename)
        except OSError:
            pass

    def testString(self):
        try:
            s = java.lang.String("test")
            with open(self.filename, "wb") as fd:
                JPickler(fd).dump(s)
            with open(self.filename, "rb") as fd:
                s2 = JUnpickler(fd).load()
        except pickle.UnpicklingError:
            dump(self.filename)
        self.assertEqual(s, s2)

    def testString2(self):
        try:
            s1 = java.lang.String("test1")
            s2 = java.lang.String("test2")
            with open(self.filename, "wb") as fd:
                pickler = JPickler(fd)
                pickler.dump(s1)
                pickler.dump(s2)
            with open(self.filename, "rb") as fd:
                unpickler = JUnpickler(fd)
                s1_ = unpickler.load()
                s2_ = unpickler.load()

            self.assertEqual(s1, s1_)
            self.assertEqual(s2, s2_)
        except pickle.UnpicklingError:
            dump(self.filename)

    def testList(self):
        s = java.util.ArrayList()
        s.add("test")
        s.add("this")
        try:
            with open(self.filename, "wb") as fd:
                JPickler(fd).dump(s)
            with open(self.filename, "rb") as fd:
                s2 = JUnpickler(fd).load()
        except pickle.UnpicklingError:
            dump(self.filename)
        self.assertEqual(s2.get(0), "test")
        self.assertEqual(s2.get(1), "this")

    def testMixed(self):
        d = {"array": java.util.ArrayList(),
             "string": java.lang.String("food")}
        try:
            with open(self.filename, "wb") as fd:
                JPickler(fd).dump(d)
            with open(self.filename, "rb") as fd:
                d2 = JUnpickler(fd).load()
        except pickle.UnpicklingError:
            dump(self.filename)
        self.assertEqual(d2['string'], "food")
        self.assertIsInstance(d2['array'], java.util.ArrayList)

    def testMultiObject(self):
        """Regression test for https://github.com/jpype-project/jpype/issues/1201

        Issue occurs when a python object contains multiple java objects above
        a certain size. The issue occurs because of a buffer overflow in
        ``native/java/org/jpype/pickle/ByteBufferInputStream.java``.
        """
        JString = jpype.JClass("java.lang.String")
        composite_object = {"a": JString('A' * 512), "b": JString('B' * 512)}

        with open(self.filename, "wb") as fd:
            JPickler(fd).dump(composite_object)

        with open(self.filename, "rb") as fd:
            loaded_object = JUnpickler(fd).load()
        
        assert loaded_object['a'] == str(composite_object['a'])
        assert loaded_object['b'] == str(composite_object['b'])

    def testByteBufferInputStream(self):
        JByteBufferInputStream = jpype.JClass("org.jpype.pickle.ByteBufferInputStream")
        stream = JByteBufferInputStream()
        bb = jpype.JClass("java.nio.ByteBuffer").allocate(10)
        stream.put(b"abc")
        stream.put(b"def")

        assert stream.read() == ord('a')
        assert stream.read(bb.array()) == 5
        assert bytes(bb.array()) == b'bcdef\x00\x00\x00\x00\x00'

    def testFail(self):
        s = java.lang.Object()
        with self.assertRaises(java.io.NotSerializableException):
            with open(self.filename, "wb") as fd:
                JPickler(fd).dump(s)
