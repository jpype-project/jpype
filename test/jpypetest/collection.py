import jpype
from . import common

class CollectionTestCase(common.JPypeTestCase):

    def setUp(self):
        super(CollectionTestCase, self).setUp()

    def testCollection(self):
        collection = jpype.java.util.ArrayList()
        collection.add(1)
        collection.add(2)
        self.assertEqual([1, 2], list(collection))
