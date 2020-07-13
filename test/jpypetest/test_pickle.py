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
from jpype import java
from jpype.pickle import JPickler, JUnpickler
import pickle
import sys
import common


import unittest


def dump(fname):
    with open(fname, "rb") as fd:
        data = fd.read()
    out = ["%02x" % i for i in data]
    print("Pickle fail", " ".join(out), file=sys.stderr)


class PickleTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testString(self):
        try:
            s = java.lang.String("test")
            with open("test.pic", "wb") as fd:
                JPickler(fd).dump(s)
            with open("test.pic", "rb") as fd:
                s2 = JUnpickler(fd).load()
        except pickle.UnpicklingError:
            dump("test.pic")
        self.assertEqual(s, s2)

    def testList(self):
        s = java.util.ArrayList()
        s.add("test")
        s.add("this")
        try:
            with open("test.pic", "wb") as fd:
                JPickler(fd).dump(s)
            with open("test.pic", "rb") as fd:
                s2 = JUnpickler(fd).load()
        except pickle.UnpicklingError:
            dump("test.pic")
        self.assertEqual(s2.get(0), "test")
        self.assertEqual(s2.get(1), "this")

    def testMixed(self):
        d = {}
        d["array"] = java.util.ArrayList()
        d["string"] = java.lang.String("food")
        try:
            with open("test.pic", "wb") as fd:
                JPickler(fd).dump(d)
            with open("test.pic", "rb") as fd:
                d2 = JUnpickler(fd).load()
        except pickle.UnpicklingError:
            dump("test.pic")
        self.assertEqual(d2['string'], "food")
        self.assertIsInstance(d2['array'], java.util.ArrayList)

    def testFail(self):
        s = java.lang.Object()
        with self.assertRaises(java.io.NotSerializableException):
            with open("test.pic", "wb") as fd:
                JPickler(fd).dump(s)
