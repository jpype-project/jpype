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
from jpype.types import *
from jpype import java
import common

class MyTest():
    def __init__(self):
        pass

class JBridgeTestCase(common.JPypeTestCase):
    """ Test for methods of java.lang.Map
    """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.fixture = JClass("jpype.common.Fixture")()
        self.Bridge = JClass("org.jpype.bridge.Interpreter")
        self.Backend = JClass("org.jpype.bridge.Backend")

        # Concrete
        self.PyByteArray = JClass("python.lang.PyByteArray")
        self.PyBytes = JClass("python.lang.PyBytes")
        self.PyComplex = JClass("python.lang.PyComplex")
        self.PyDict = JClass("python.lang.PyDict")
        self.PyEnumerate = JClass("python.lang.PyEnumerate")
        self.PyJavaObject = JClass("python.lang.PyJavaObject")
        self.PyList = JClass("python.lang.PyList")
        self.PyMemoryView = JClass("python.lang.PyMemoryView")
        self.PyObject = JClass("python.lang.PyObject")
        self.PyRange = JClass("python.lang.PyRange")
        self.PySet = JClass("python.lang.PySet")
        self.PySlice = JClass("python.lang.PySlice")
        self.PyString = JClass("python.lang.PyString")
        self.PyTuple = JClass("python.lang.PyTuple")
        self.PyType = JClass("python.lang.PyType")
        self.PyZip = JClass("python.lang.PyZip")

        # Protocols
        self.PyIter = JClass("python.protocol.PyIter")
        self.PyIterable = JClass("python.protocol.PyIterable")
        self.PyAttributes = JClass("python.protocol.PyAttributes")
        self.PyMapping = JClass("python.protocol.PyMapping")
        self.PyNumber = JClass("python.protocol.PyNumber")
        self.PySequence = JClass("python.protocol.PySequence")

    def testByteArray(self):
        obj = bytearray()
        self.assertEqual(self.PyByteArray._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyByteArray@obj, self.PyByteArray)

    def testBytes(self):
        obj = bytes()
        self.assertEqual(self.PyBytes._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyBytes@obj, self.PyBytes)

    def testComplex(self):
        obj = complex(1,2)
        self.assertEqual(self.PyComplex._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyComplex@obj, self.PyComplex)

    def testDict(self):
        obj = {"a":1, "b":2}
        self.assertEqual(self.PyDict._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyDict@obj, self.PyDict)

    def testEnumerate(self):
        l = list([1,2,3])
        obj = enumerate(l)
        self.assertEqual(self.PyEnumerate._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyEnumerate@obj, self.PyEnumerate)

    def testIterable(self):
        obj = list()
        self.assertEqual(self.PyIterable._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyIterable@obj, self.PyIterable)

    def testIter(self):
        obj = iter([1,2,3,4])
        self.assertEqual(self.PyIter._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyIter@obj, self.PyIter)
        
        def even(s):
            return s%2==0

        def keep(s):
            return True

        jobj = self.PyIter@iter([1,2,3,4])
        self.assertEqual(list(jobj.filter(even)), [2,4])

        jobj = self.PyIter@iter([1,2,3,4])
        self.assertEqual(jobj.next(), 1)
        self.assertEqual(jobj.next(), 2)
        self.assertEqual(list(jobj.filter(keep)), [3,4])

        jobj = self.PyIter@iter([1,2,3,4])
        jiter1 = jobj.iterator()
# FIXME return type issue
#        l = []
#        while jiter1.hasNext():
#            jiter1.append(jiter1.next())
#        self.assertEqual(l, [1,2,3,4])
            
        

    def testList(self):
        obj = list()
        self.assertEqual(self.PyList._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyList@obj, self.PyList)

    def testMemoryView(self):
        obj = memoryview(bytes())
        self.assertEqual(self.PyMemoryView._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyMemoryView@obj, self.PyMemoryView)

    def testObject(self):
        obj = object()
        self.assertEqual(self.PyObject._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyObject@obj, self.PyObject)

    def testRange(self):
        obj = range(5)
        self.assertEqual(self.PyRange._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyRange@obj, self.PyRange)

    def testSet(self):
        self.assertEqual(self.PySet._canConvertToJava(set()), "implicit")
        self.assertIsInstance(self.PySet@set(), self.PySet)

    def testSlice(self):
        obj = slice(1,5,None)
        self.assertEqual(self.PySlice._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PySlice@obj, self.PySlice)

    def testString(self):
        obj = "hello"
        self.assertEqual(self.PyString._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyString@obj, self.PyString)

    def testTuple(self):
        obj = (1,2,3)
        self.assertEqual(self.PyTuple._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyTuple@obj, self.PyTuple)

    def testType(self):
        obj = type(dict())
        self.assertEqual(self.PyType._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyType@obj, self.PyType)

    def testZip(self):
        l1 = ('a','b','c')
        l2 = (1,2,3)
        obj = zip(l1, l2)
        self.assertEqual(self.PyZip._canConvertToJava(obj), "implicit")
        self.assertIsInstance(self.PyZip@obj, self.PyZip)

    def testBackend(self):
        built = JClass("org.jpype.bridge.BuiltIn")

        self.assertIsInstance(built.list(java.util.Arrays.asList("A","B","C")), list)
        self.assertIsInstance(built.list("A","B","C"), list)
        self.assertIsInstance(built.str(self.PyString@"hello"), str)

        o = MyTest()
        setattr(o, "field", "AA")
        self.assertTrue(built.hasattr(o, "field"))
        self.assertEqual(built.getattr(o, "field"), "AA")
        built.setattr(o, "field", "BB")
        self.assertEqual(o.field, "BB")
        built.delattr(o, "field")
        self.assertFalse(hasattr(o, "field"))
        self.assertTrue(built.isinstance(int(), self.PyObject[:]((int, float))))
        self.assertFalse(built.isinstance(str(), self.PyObject[:]((int, float))))

        #==========================================================
        # Test the object behavior

    def testAttributes(self):
        o = MyTest()
        setattr(o, "A", 1)
        setattr(o, "B", 2)
        j = self.PyObject@o
        a = self.PyAttributes@(j.asAttributes())
        #print(dir(a))
        self.assertEqual(a.get("A"), 1)
        self.assertEqual(a.get("B"), 2)
        a.set("B",3)  # Note we end up with a Java Integer here
        self.assertTrue(a.has("B")) 
        self.assertEqual(a.get("B").get(), 3)  # Use get() to unwrap the Integer
        self.assertFalse(a.has("C")) 
        self.assertEqual(a.dict(), {"A":1, "B":3}) 
        a.remove("B")  
        self.assertFalse(a.has("B"))


