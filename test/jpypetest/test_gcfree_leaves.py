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
import gc
import pickle

import _jpype
import jpype
import common
from jpype.types import *

# Py_TPFLAGS_HAVE_GC
_HAVE_GC = 1 << 14

_LEAVES = (JBoolean, JByte, JChar, JInt, JShort, JLong, JFloat, JDouble)


class GcFreeLeavesTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testNotGCTracked(self):
        for cls in _LEAVES:
            with self.subTest(cls=cls):
                self.assertFalse(cls.__flags__ & _HAVE_GC)

    def testIdentityWithModuleAttribute(self):
        for cls in _LEAVES:
            with self.subTest(cls=cls):
                self.assertIs(getattr(_jpype, cls.__name__), cls)

    def testModuleNameRepr(self):
        for cls in _LEAVES:
            with self.subTest(cls=cls):
                self.assertEqual(cls.__module__, "jpype.types")
                self.assertEqual(repr(cls), "<java class '%s'>" % cls.__name__)

    def testCannotExtend(self):
        for cls in _LEAVES:
            with self.subTest(cls=cls):
                with self.assertRaises(TypeError):
                    type("Sub" + cls.__name__, (cls,), {})

    def testInstancesNotTracked(self):
        self.assertFalse(gc.is_tracked(JInt(5)))
        self.assertFalse(gc.is_tracked(JLong(5)))
        self.assertFalse(gc.is_tracked(JBoolean(True)))
        self.assertFalse(gc.is_tracked(JChar('a')))

    def testArrayPullNotTracked(self):
        arr = JArray(JInt)(5)
        for i in range(5):
            arr[i] = i * i
        values = list(arr)
        self.assertEqual(values, [0, 1, 4, 9, 16])
        for v in values:
            self.assertFalse(gc.is_tracked(v))
            self.assertIsInstance(v, JInt)

    def testSizeofMatchesPlain(self):
        self.assertEqual(JInt(5).__sizeof__(), (5).__sizeof__())
        self.assertEqual(JLong(5).__sizeof__(), (5).__sizeof__())

    def testRoundTrip(self):
        self.assertEqual(int(JInt(5)), 5)
        self.assertEqual(JInt(5), 5)
        self.assertEqual(hash(JInt(5)), hash(5))
        self.assertEqual(JInt(5) + JInt(2), 7)
        self.assertTrue(JBoolean(True))
        self.assertFalse(JBoolean(False))

    def testStressUnderGCPressure(self):
        arr = JArray(JInt)(10000)
        gc.collect()
        values = list(arr)
        self.assertEqual(len(values), 10000)
        del values
        gc.collect()

    def testPickle(self):
        for value in (JInt(5), JLong(5), JBoolean(True)):
            with self.subTest(value=value):
                self.assertEqual(pickle.loads(pickle.dumps(value)), value)
