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
from jpype.types import *
import common
import collections.abc


class CollectionCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    @common.requirePythonAfter((3, 6, 0))
    def testCollectionABC(self):
        JCollection = JClass('java.util.Collection')
        self.assertFalse(issubclass(JCollection, collections.abc.Sequence))
        self.assertFalse(issubclass(JCollection, collections.abc.Reversible))
        self.assertTrue(issubclass(JCollection, collections.abc.Collection))
        self.assertTrue(issubclass(JCollection, collections.abc.Iterable))
        self.assertTrue(issubclass(JCollection, collections.abc.Sized))

    def testCollectionDelItem(self):
        ja = JClass('java.util.ArrayList')(['1', '2', '3'])
        jc = JObject(ja, 'java.util.Collection')
        with self.assertRaisesRegex(TypeError, 'remove'):
            del jc[1]

    def testListAddAll(self):
        l = [1, 2, 3, 4]
        l2 = ['a', 'b']
        jlist = JClass("java.util.ArrayList")()
        jlist.addAll(l)
        jcollection = JObject(jlist, JClass("java.util.Collection"))
        jcollection.addAll(l2)
        l.extend(l2)
        self.assertEqual(l, list(jcollection))


class CollectionIteratorCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testIterator(self):
        al = JClass("java.util.ArrayList")()
        itr = al.iterator()
        self.assertEqual(itr, iter(itr))


class CollectionListCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testListAdd(self):
        collection = JClass("java.util.ArrayList")()
        collection.add(1)
        collection.add(2)
        self.assertEqual([1, 2], [i for i in collection])

    def testListGet(self):
        jlist = JClass("java.util.ArrayList")()
        jlist.addAll([1, 2, 3, 4])
        self.assertEqual(jlist[0], 1)
        self.assertEqual(jlist[3], 4)
        self.assertEqual(jlist[-1], 4)
        self.assertEqual(jlist[-4], 1)

    def testListSlice(self):
        jlist = JClass("java.util.ArrayList")()
        jlist.addAll([1, 2, 3, 4])
        jlist[1:3] = [5, 6]
        self.assertEqual(jlist[1], 5)
        self.assertEqual(jlist[2], 6)

    def testListDel(self):
        jlist = JClass("java.util.ArrayList")()
        jlist.addAll([1, 2, 3, 4])
        del jlist[0]
        self.assertEqual(len(jlist), 3)
        self.assertEqual(jlist[0], 2)

    def testListSetItemNeg(self):
        l = [1, 2, 3, 4]
        jlist = JClass("java.util.ArrayList")()
        jlist.addAll([1, 2, 3, 4])
        jlist[-1] = 5
        l[-1] = 5
        self.assertEqual(l, list(jlist))
        jlist[-2] = 6
        l[-2] = 6
        self.assertEqual(l, list(jlist))
        with self.assertRaises(IndexError):
            jlist[-5] = 6

    def testListIter(self):
        ls = JClass('java.util.ArrayList')([0, 1, 2, 3])
        for i, j in enumerate(ls):
            self.assertEqual(i, j)

    def testUnmodifiableNext(self):
        ArrayList = JClass('java.util.ArrayList')
        Collections = JClass('java.util.Collections')
        a = ArrayList()
        a.add("first")
        a.add("second")
        a.add("third")
        for i in a:
            pass
        for i in Collections.unmodifiableList(a):
            pass

    @common.requirePythonAfter((3, 6, 0))
    def testListABC(self):
        l = ['a', 'b', 'c', 'b']
        JList = JClass('java.util.ArrayList')
        al = JList(l)
        for i, j in zip(reversed(al), reversed(l)):
            self.assertEqual(i, j)
        self.assertEqual(object() in al, object() in l)
        self.assertEqual('a' in al, 'a' in l)
        self.assertEqual(al.index('b'), l.index('b'))
        self.assertEqual(al.count('b'), l.count('b'))
        with self.assertRaises(ValueError):
            al.index(object())
        self.assertEqual(al.count(object()), l.count(object()))
        self.assertIsInstance(al, collections.abc.Sequence)
        self.assertIsInstance(al, collections.abc.Reversible)
        self.assertIsInstance(al, collections.abc.Collection)
        self.assertIsInstance(al, collections.abc.Iterable)
        self.assertIsInstance(al, collections.abc.Sized)


class CollectionMapCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testIterateMap(self):
        collection = JClass('java.util.HashMap')()
        collection.put('A', 1)
        collection.put('B', 2)
        asdict = dict()
        for x in collection.entrySet():
            asdict[str(x.getKey())] = x.getValue().longValue()
        self.assertEqual(asdict, {'A': 1, 'B': 2})

    def testMapPut(self):
        jmap = JClass("java.util.HashMap")()
        jmap["a"] = 1
        self.assertEqual(jmap["a"], 1)

    def testMapPutAll(self):
        jmap = JClass("java.util.HashMap")()
        dic = {"a": "1", "b": "2", "c": "3"}
        jmap.putAll(dic)
        self.assertEqual(jmap["a"], "1")
        self.assertEqual(jmap["b"], "2")
        self.assertEqual(jmap["c"], "3")
        with self.assertRaises(TypeError):
            jmap.putAll([1, 2, 3])

    def testMapKeyError(self):
        hm = JClass('java.util.HashMap')()
        with self.assertRaises(KeyError):
            hm['foo']
        hm['foo'] = None
        self.assertEqual(hm['foo'], None)

    def testHashMapEntryIter(self):
        hm = JClass('java.util.HashMap')()
        hm['alice'] = 'alice'
        hm['betty'] = 'betty'
        hm['catty'] = 'catty'
        for p, v in hm.entrySet():
            self.assertEqual(p, v)

    def testTreeMapEntryIter(self):
        hm = JClass('java.util.TreeMap')()
        hm['alice'] = 'alice'
        hm['betty'] = 'betty'
        hm['catty'] = 'catty'
        for p, v in hm.entrySet():
            self.assertEqual(p, v)

    def testMapEntry(self):
        hm = JClass('java.util.TreeMap')()
        hm['alice'] = 'alice'
        h = hm.entrySet()
        self.assertEqual(len(h.iterator().next()), 2)

    def testEnumMap(self):
        enumclass = JClass('jpype.collection.TestEnum')
        enummap = JClass('java.util.EnumMap')(enumclass)
        enummap.put(enumclass.A, 'ABC')
        enummap.put(enumclass.B, 'DEF')
        asdict = dict()
        for x in enummap.entrySet():
            asdict[str(x.getKey())] = x.getValue()
        self.assertEqual({'A': 'ABC', 'B': 'DEF'}, asdict)

    def testSetDelItem(self):
        hs = JClass('java.util.HashSet')()
        hs.add('a')
        hs.add('b')
        hs.add('c')
        self.assertIn('a', hs)
        del hs['a']
        self.assertNotIn('a', hs)

    def testHashMapCtor(self):
        HashMap = JClass('java.util.HashMap')
        dc = dict()
        dc['fred'] = 1
        dc['george'] = 2
        dc['paul'] = 3
        hm = HashMap(dc)
        for p, v in dc.items():
            self.assertEqual(hm[p], v)

    def testHashMapPutAll(self):
        HashMap = JClass('java.util.HashMap')
        hm = HashMap()
        dc = dict()
        dc['fred'] = 1
        dc['george'] = 2
        dc['paul'] = 3
        hm.putAll(dc)
        for p, v in dc.items():
            self.assertEqual(hm[p], v)

    def testHashMapConvert(self):
        HashMap = JClass('java.util.HashMap')
        hm = HashMap()
        hm['fred'] = 1
        hm['george'] = 2
        hm['paul'] = 3
        dc = dict(hm)
        for p, v in hm.items():
            self.assertEqual(dc[p], v)

    def testMapABC(self):
        from collections.abc import Mapping, Sized, Iterable, Container
        hm = JClass('java.util.HashMap')()
        self.assertIsInstance(hm, Sized)
        self.assertIsInstance(hm, Iterable)
        self.assertIsInstance(hm, Container)
        self.assertIsInstance(hm, Mapping)

    def testMapContains(self):
        hm = JClass('java.util.HashMap')()
        hm['fred'] = 1
        hm['george'] = 2
        hm['paul'] = 3
        self.assertTrue("fred" in hm)
        self.assertFalse("sally" in hm)

    def testMapNoConversion(self):
        hm = JClass("java.util.HashMap")()
        self.assertFalse(object() in hm)
        with self.assertRaises(KeyError):
            hm[object()]


class CollectionEnumerationCase(common.JPypeTestCase):

    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testEnumeration(self):
        st = JClass('java.util.StringTokenizer')("this is a test")
        out = []
        for i in st:
            out.append(str(i))
        self.assertEqual(len(i), 4)
        self.assertEqual(" ".join(out), "this is a test")
