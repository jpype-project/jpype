import jpype
from . import common

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
        collection.put('A',1)
        collection.put('B',2)
        asdict = dict()
        for x in collection.entrySet():
            asdict[x.getKey()] = x.getValue().longValue()
        self.assertEqual({'A':1,'B':2}, asdict)
                    
    def testEnumMap(self):
        enumclass = jpype.JClass('jpype.collection.TestEnum')
        enummap = jpype.java.util.EnumMap(enumclass)
        enummap.put(enumclass.A, 'ABC')
        enummap.put(enumclass.B, 'DEF')
        asdict = dict()
        for x in enummap.entrySet():
            asdict[str(x.getKey())] = x.getValue()
        self.assertEqual({'A':'ABC','B':'DEF'}, asdict)