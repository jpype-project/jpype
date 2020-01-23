# *****************************************************************************
#   Copyright 2019 Karl Einar Nelson
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
import jpype
import common


class JListTestCase(common.JPypeTestCase):
    """ Test for methods of java.lang.Map 

    def __getitem__(self, ndx):
        if isinstance(ndx, slice):
            start = ndx.start
            stop = ndx.stop
            if start < 0:
                start = self.size() + start
            if stop < 0:
                stop = self.size() + stop
            return self.subList(start, stop)
        else:
            if ndx < 0:
                ndx = self.size() + ndx
            return self.get(ndx)

    def __setitem__(self, ndx, v):
        if isinstance(ndx, slice):
            start = ndx.start
            stop = ndx.stop
            if start < 0:
                start = self.size() + start
            if stop < 0:
                stop = self.size() + stop
            for i in range(start, stop):
                self.remove(start)
            if isinstance(v, collections.abc.Sequence):
                ndx = start
                for i in v:
                    self.add(ndx, i)
                    ndx += 1
        else:
            if ndx < 0:
                ndx = self.size() + ndx
            self.set(ndx, v)
            """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.cls = jpype.JClass('java.util.ArrayList')
        self.Arrays = jpype.JClass('java.util.Arrays')

    def testLen(self):
        obj = self.cls()
        obj.add(1)
        obj.add(2)
        obj.add(3)
        self.assertEqual(len(obj), 3)

    def testIter(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        self.assertEqual(tuple(i for i in obj), ('a', 'b', 'c'))

    def testGetItem(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        self.assertEqual(obj[1], 'b')

    def testGetItemSub(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        obj.add("d")
        self.assertEqual(tuple(i for i in obj[1:3]), ('b', 'c'))

    def testGetItemSlice(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        obj.add("d")
        self.assertEqual(tuple(obj[::1]), ('a', 'b', 'c', 'd'))
        self.assertEqual(tuple(obj[:-2]), ('a', 'b'))
        self.assertEqual(tuple(obj[-2:]), ('c', 'd'))
        with self.assertRaises(TypeError):
            obj[::2]
        with self.assertRaises(TypeError):
            obj[::-1]

    def testRemoveRange(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        obj.add("d")
        obj[1:3].clear()
        self.assertEqual(tuple(i for i in obj), ('a', 'd'))

    def testSetItem(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        obj[1] = 'changed'
        self.assertEqual(tuple(i for i in obj), ('a', 'changed', 'c'))

    def testDelItem(self):
        obj = self.cls()
        obj.add("a")
        obj.add("b")
        obj.add("c")
        del obj[1]
        self.assertEqual(tuple(i for i in obj), ('a', 'c'))
        obj.add("a")
        obj.add("b")
        obj.add("c")
        del obj[1:3]
        with self.assertRaises(TypeError):
            del obj[1.0]

    def testAddAll(self):
        obj = self.cls()
        obj.addAll(["a", "b", "c"])
        self.assertEqual(tuple(i for i in obj), ('a', 'b', 'c'))
        obj.addAll(1, ["a", "b", "c"])
        self.assertEqual(tuple(i for i in obj), ('a', 'a', 'b', 'c', 'b', 'c'))
        with self.assertRaises(TypeError):
            obj.addAll()
        with self.assertRaises(TypeError):
            obj.addAll(1, 2, 3)
        with self.assertRaises(TypeError):
            obj.addAll(1, 2)
        with self.assertRaises(TypeError):
            obj.addAll(1.0, ['a'])

    def testRemoveAll(self):
        obj = self.cls()
        obj.addAll(["a", "b", "c", "d", "e"])
        obj.removeAll(["c", "d"])
        self.assertEqual(tuple(i for i in obj), ('a', 'b', 'e'))

    def testRetainAll(self):
        obj = self.cls()
        obj.addAll(["a", "b", "c", "d", "e"])
        obj.retainAll(["c", "d"])
        self.assertEqual(tuple(i for i in obj), ('c', 'd'))

    def testInit(self):
        cls = jpype.JClass('java.util.ArrayList')
        obj = cls(self.Arrays.asList(['a', 'b', 'c']))
        self.assertEqual(tuple(i for i in obj), ('a', 'b', 'c'))
