import jpype
import common

class CollectionTestCase(common.JPypeTestCase):

    def setUp(self):
        super(CollectionTestCase, self).setUp()

    def testCollection(self):
        collection = jpype.java.util.ArrayList()
        collection.add(1)
        collection.add(2)
        self.assertEqual([1, 2], [i for i in collection])

    def testIterateHashmap(self):
        collection = jpype.java.util.HashMap()
        collection.put('A', 1)
        collection.put('B', 2)
        asdict = dict()
        for x in collection.entrySet():
            asdict[str(x.getKey())] = x.getValue().longValue()
        self.assertEqual(asdict, {'A': 1, 'B': 2})

    def testEnumMap(self):
        enumclass = jpype.JClass('jpype.collection.TestEnum')
        enummap = jpype.java.util.EnumMap(enumclass)
        enummap.put(enumclass.A, 'ABC')
        enummap.put(enumclass.B, 'DEF')
        asdict = dict()
        for x in enummap.entrySet():
            asdict[str(x.getKey())] = x.getValue()
        self.assertEqual({'A': 'ABC', 'B': 'DEF'}, asdict)

    def testMapPut(self):
        jmap = jpype.JClass("java.util.HashMap")()
        jmap["a"]=1
        self.assertEqual(jmap["a"],1)

    def testMapPutAll(self):
        jmap = jpype.JClass("java.util.HashMap")()
        dic = {"a":"1", "b":"2", "c":"3"}
        jmap.putAll(dic)
        self.assertEqual(jmap["a"],"1")
        self.assertEqual(jmap["b"],"2")
        self.assertEqual(jmap["c"],"3")
        with self.assertRaises(TypeError):
            jmap.putAll([1,2,3])

    def testListGet(self):
        jlist = jpype.JClass("java.util.ArrayList")()
        jlist.addAll([1,2,3,4])
        self.assertEqual(jlist[0],1)
        self.assertEqual(jlist[3],4)
        self.assertEqual(jlist[-1],4)
        self.assertEqual(jlist[-4],1)

    def testListSlice(self):
        jlist = jpype.JClass("java.util.ArrayList")()
        jlist.addAll([1,2,3,4])
        jlist[1:3]=[5,6]
        self.assertEqual(jlist[1],5)
        self.assertEqual(jlist[2],6)

    def testListDel(self):
        jlist = jpype.JClass("java.util.ArrayList")()
        jlist.addAll([1,2,3,4])
        del jlist[0]
        self.assertEqual(len(jlist),3)
        self.assertEqual(jlist[0],2)


