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
import _jpype
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
        self.Bridge = JClass("org.jpype.MainInterpreter")
        self.Backend = JClass("org.jpype.Backend")

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
        self.PyCallable = JClass("python.lang.PyCallable")

        # Protocols
        self.PyIter = JClass("python.lang.PyIter")
        self.PyIterable = JClass("python.lang.PyIterable")
        self.PyAttributes = JClass("python.lang.PyAttributes")
        self.PyMapping = JClass("python.lang.PyMapping")
        self.PyNumber = JClass("python.lang.PyNumber")
        self.PySequence = JClass("python.lang.PySequence")

        self.PyBuiltIn = self.Bridge.getInstance().getBuiltIn()
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

    def testBackend1(self):
        self.assertIsInstance(self.PyBuiltIn.list(java.util.Arrays.asList("A","B","C")), list)
        self.assertIsInstance(self.PyBuiltIn.list(self.PyObject@["A","B","C"]), list)
        self.assertIsInstance(self.PyBuiltIn.str(self.PyString@"hello"), str)

    def testBackend2(self):
        o = MyTest()
        setattr(o, "field", "AA")
        self.assertTrue(self.PyBuiltIn.hasattr(o, "field"))
        self.assertEqual(self.PyBuiltIn.getattr(o, "field"), "AA")
        self.PyBuiltIn.setattr(o, "field", "BB")
        self.assertEqual(o.field, "BB")
        self.PyBuiltIn.delattr(o, "field")
        self.assertFalse(hasattr(o, "field"))

    def testBackend3(self):
        self.assertTrue(self.PyBuiltIn.isinstance(int(), self.PyTuple@(int, float)))
        self.assertFalse(self.PyBuiltIn.isinstance(str(), self.PyTuple@(int, float)))

    def testAttributes(self):
        o = MyTest()
        setattr(o, "A", 1)
        setattr(o, "B", 2)
        j = self.PyObject@o
        a = j.getAttributes()
        #print(dir(a))
        self.assertEqual(a.get("A"), 1)
        self.assertEqual(a.get("B"), 2)
        a.put("B",3)  # Note we end up with a Java Integer here
        self.assertTrue(a.contains("B")) 
        self.assertEqual(a.get("B"), 3)
        self.assertFalse(a.contains("C")) 
        self.assertEqual(a.asDict(), {"A":1, "B":3}) 
        a.remove("B")
        self.assertFalse(a.contains("B"))

    def testExc(self):
        exc = _jpype._pyexc_convert(ValueError("nbar"))
        cls = JClass("python.exceptions.PyValueError")
        self.assertIsInstance(exc, cls)

    def testExcBad(self):
        with self.assertRaises(JClass("python.exceptions.PyAssertionError")):
            raise _jpype._pyexc_convert(object())

class TestPyBuiltInSuite(common.JPypeTestCase):
    """ Comprehensive behavioral unit tests for python.lang.PyBuiltIn """

    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.Bridge = JClass("org.jpype.MainInterpreter")
        self.PyBuiltIn = self.Bridge.getInstance().getBuiltIn()
        
        # Add the missing proxy classes your tests need!
        self.PyObject = JClass("python.lang.PyObject")
        self.PyString = JClass("python.lang.PyString")
        self.PyInt = JClass("python.lang.PyInt")
        self.PyFloat = JClass("python.lang.PyFloat")
        self.PySlice = JClass("python.lang.PySlice")
        self.PyRange = JClass("python.lang.PyRange")
        self.PyDict = JClass("python.lang.PyDict")
        self.PyTuple = JClass("python.lang.PyTuple")
        self.PyBytes = JClass("python.lang.PyBytes")
        self.PyList = JClass("python.lang.PyList")
        self.PySubscript = JClass("python.lang.PySubscript")
        self.PyCallable = JClass("python.lang.PyCallable")
        self.Integer = JClass("java.lang.Integer")

    # ---------------------------------------------------------
    # 1. Numeric Primitives ($int, $float)
    # ---------------------------------------------------------
    def test_numeric_instantiation(self):
        # Test $int(long value)
        j_int = getattr(self.PyBuiltIn, "$int")(8675309)
        self.assertIsInstance(j_int, int)
        self.assertEqual(int(j_int), 8675309)

        # Test $float(double value)
        j_float = getattr(self.PyBuiltIn, "$float")(2.71828)
        self.assertIsInstance(j_float, float)
        self.assertAlmostEqual(float(j_float), 2.71828, places=5)

    # ---------------------------------------------------------
    # 2. Slice Overloads
    # ---------------------------------------------------------
    def test_slice_one_arg(self):
        # slice(int start) -> equivalent to [start:start+1]
        j_slice = self.PyBuiltIn.slice(5)
        self.assertIsInstance(j_slice, slice)
        j_slice = self.PySlice@j_slice
        self.assertEqual(j_slice.getStart(), 5)
        self.assertEqual(j_slice.getStop(), 6)
        self.assertIsNone(j_slice.getStep())

    def test_slice_two_args(self):
        # slice(Integer start, Integer stop)
        j_slice = self.PyBuiltIn.slice(JInt(2), JInt(10))
        j_slice = self.PySlice@j_slice
        self.assertEqual(j_slice.getStart(), 2)
        self.assertEqual(j_slice.getStop(), 10)
        self.assertIsNone(j_slice.getStep())

        # Passing None to mimic null references in Java signatures
        j_slice_nulls = self.PyBuiltIn.slice(None, JInt(-1))
        j_slice_nulls = self.PySlice@j_slice_nulls
        self.assertIsNone(j_slice_nulls.getStart())
        self.assertEqual(j_slice_nulls.getStop(), -1)

    def test_slice_three_args(self):
        # slice(Integer start, Integer stop, Integer step)
        j_slice = self.PyBuiltIn.slice(JInt(0), JInt(20), JInt(2))
        j_slice = self.PySlice@j_slice
        self.assertEqual(j_slice.getStart(), 0)
        self.assertEqual(j_slice.getStop(), 20)
        self.assertEqual(j_slice.getStep(), 2)

    # ---------------------------------------------------------
    # 3. Range Overloads
    # ---------------------------------------------------------
    def test_range_overloads(self):
        # range(int stop)
        r1 = self.PyBuiltIn.range(10)
        self.assertIsInstance(r1, range)
        
        # range(int start, int stop)
        r2 = self.PyBuiltIn.range(1, 10)
        self.assertIsInstance(r2, range)

        # range(int start, int stop, int step)
        r3 = self.PyBuiltIn.range(1, 10, 2)
        self.assertIsInstance(r3, range)

    # ---------------------------------------------------------
    # 4. Evaluation and Execution Blocks (eval / exec)
    # ---------------------------------------------------------
    def test_eval_expression(self):
        globals_dict = self.PyBuiltIn.dict()
        expression = "sum([1, 2, 3])"

        # 1. The Success Path: Passing a valid mapping for locals
        locals_dict = self.PyBuiltIn.dict()
        result = self.PyBuiltIn.eval(expression, globals_dict, locals_dict)
        self.assertEqual(int(result), 6)

        # 2. Passing None for locals mirrors eval("...", dict(), None), which
        # real Python's eval() accepts (locals defaults to globals) - the
        # bridge must deliver a genuine null as real Python None here, not a
        # null-backed typed wrapper (see plan/FunctionRetrieval.md).
        result = self.PyBuiltIn.eval(expression, globals_dict, None)
        self.assertEqual(int(result), 6)

    def test_exec_statement(self):
        globals_dict = self.PyBuiltIn.dict()
        locals_mapping = self.PyBuiltIn.dict() # assuming it complies with PyMapping
        
        # Python mutations that run purely inside the context
        statement = "x = 40 + 2"
        self.PyBuiltIn.exec_(statement, globals_dict, locals_mapping)
        
        # The result should have modified our namespace mapping
        self.assertEqual(int(locals_mapping.get("x")), 42)

    # ---------------------------------------------------------
    # 5. Core Built-In Functions (len, vars, dir)
    # ---------------------------------------------------------
    def test_len(self):
        test_list = ["apple", "banana", "cherry"]
        self.assertEqual(self.PyBuiltIn.len(test_list), 3)

        # Expected error path if type lacks len() capability
        with self.assertRaises(Exception):
            self.PyBuiltIn.len(12345)

    def test_vars_and_dir(self):
        class Dummy:
            def __init__(self):
                self.foo = "bar"
        
        d = Dummy()
        # Verify vars returns the internal dict namespace
        j_vars = self.PyBuiltIn.vars(d)
        j_vars = self.PyDict@j_vars
        self.assertIsInstance(j_vars, self.PyDict)
        self.assertEqual(j_vars.get("foo"), "bar")

        # Verify dir pulls attribute definitions
        j_dir = self.PyBuiltIn.dir(d)
        self.assertTrue("foo" in list(j_dir))

    # ---------------------------------------------------------
    # 6. Collection & Sequence Instantiations (list, tuple, dict, set)
    # ---------------------------------------------------------
    def test_list_overloads(self):
        # list() -> empty PyList
        j_list_empty = self.PyBuiltIn.list()
        self.assertEqual(len(list(j_list_empty)), 0)

        # list(Object object) -> converted PyList
        py_source = [1, 2, 3]
        j_list_filled = self.PyBuiltIn.list(self.PyList@py_source)
        self.assertEqual(list(j_list_filled), [1, 2, 3])

    def test_tuple_overloads(self):
        # tuple() -> empty PyTuple
        j_tuple_empty = self.PyBuiltIn.tuple()
        self.assertEqual(len(tuple(j_tuple_empty)), 0)

        # tuple(Object... items) -> varargs array handling
        # Note: Passing individual arguments maps to Java's varargs automatically in JPype
        j_tuple_filled = self.PyBuiltIn.tuple("x", "y", "z")
        self.assertEqual(tuple(j_tuple_filled), ("x", "y", "z"))

    def test_set_and_dict_initializers(self):
        # dict()
        j_dict = self.PyBuiltIn.dict()
        self.assertEqual(len(j_dict.keys()), 0)

        # set()
        j_set = self.PyBuiltIn.set()
        self.assertEqual(len(j_set), 0)

    # ---------------------------------------------------------
    # 7. Iterators, Generators, and Advanced Iteration Loops
    # ---------------------------------------------------------
    def test_enumerate_overloads(self):
        source_list = self.PyObject@["a", "b"]

        # enumerate(PyObject obj) or enumerate(Iterable obj)
        # Testing both Java pathways by passing a standard list wrapper
        j_enum = self.PyBuiltIn.enumerate(source_list)
        result = [tuple(item) for item in j_enum]
        self.assertEqual(result, [(0, "a"), (1, "b")])

    def test_iter_and_next_protocol(self):
        source = [10, 20]
        # iter(Object obj)
        j_iter = self.PyBuiltIn.iter(self.PyList@source)

        # Create a proxy/sentinel object for the stop condition
        stop_sentinel = object()

        # next(PyIter iter, PyObject stop) -> returns elements sequentially
        self.assertEqual(int(self.PyBuiltIn.next(j_iter, stop_sentinel)), 10)
        self.assertEqual(int(self.PyBuiltIn.next(j_iter, stop_sentinel)), 20)
        
        # Next call should hit exhaustion and return our exact sentinel token
        end_token = self.PyBuiltIn.next(j_iter, stop_sentinel)
        self.assertEqual(end_token, stop_sentinel)

    def test_zip_varargs_iterable(self):
        list_a = ["one", "two"]
        list_b = [1, 2]

        # zip(Iterable... objects)
        j_zip = self.PyBuiltIn.zip(list_a, list_b)
        result = [tuple(item) for item in j_zip]
        self.assertEqual(result, [("one", 1), ("two", 2)])

    # ---------------------------------------------------------
    # 8. Advanced Structural Reflected Methods
    # ---------------------------------------------------------
    def test_getattr_with_default(self):
        class Target:
            def __init__(self):
                self.existing = "found"

        obj = Target()
        default_val = self.PyString@"fallback"

        # Key exists -> should return the string value
        res1 = self.PyBuiltIn.getattrDefault(obj, "existing", default_val)
        self.assertEqual(str(res1), "found")

        # Key does not exist -> should safely return default token instead of throwing
        res2 = self.PyBuiltIn.getattrDefault(obj, "missing", default_val)
        self.assertEqual(str(res2), "fallback")

    def test_indices_varargs(self):
        # indices(PySubscript... indices)
        # This takes specialized array-subscripts and generates index structures
        # To test the Java bridge signature acceptance:
        subscript1 = self.PySubscript@self.PyBuiltIn.slice(self.Integer@0, self.Integer@2)
        subscript2 = self.PySubscript@self.PyBuiltIn.slice(self.Integer@1, self.Integer@5)

        j_indices = self.PyBuiltIn.indices(subscript1, subscript2)
        self.assertIsInstance(j_indices, tuple)
        self.assertEqual(len(j_indices), 2)

    def test_memoryview_and_bytes(self):
        raw_data = b"hello"
        
        # bytes(Object obj)
        j_bytes = self.PyBuiltIn.bytes(raw_data)
        self.assertEqual(bytes(j_bytes), b"hello")

        # memoryview(Object obj)
        j_mv = self.PyBuiltIn.memoryview(self.PyBytes@j_bytes)
        self.assertEqual(j_mv.readonly, True) # simple property check on memoryview object

    def test_type_and_repr(self):
        # type(Object obj)
        j_type = self.PyBuiltIn.type(self.PyObject@"abc")
        self.assertEqual(j_type, type("abc"))

        # repr(Object obj)
        j_repr = self.PyBuiltIn.repr(self.PyList@[1, 2])
        self.assertEqual(str(j_repr), "[1, 2]")

        # str(Object obj) - Added to explicitly test casting raw primitives
        j_str = self.PyBuiltIn.str(self.PyInt@12345)
        self.assertEqual(str(j_str), "12345")
 
    # ---------------------------------------------------------
    # 9. Low-Level Bridge Protocol & Core Invocation Loops
    # ---------------------------------------------------------
    def test_primitive_extractions(self):
        # Testing Package-Private unpackers: asLong and asDouble
        j_int_obj = self.PyInt@999
        j_float_obj = self.PyFloat@45.67

        # Ensure extraction maps down precisely to Python primitives
        self.assertEqual(self.PyBuiltIn.asLong(j_int_obj), 999)
        self.assertAlmostEqual(self.PyBuiltIn.asDouble(j_float_obj), 45.67, places=2)

    def test_callable_invocation_engine(self):
        # Construct a dummy callable target in Python
        def sample_function(x, y, multiplier=1):
            return (x + y) * multiplier

        # Wrap structural components into their required Java types
        j_callable = self.PyCallable@sample_function
        j_args = self.PyBuiltIn.tuple(10, 5)
        
        j_kwargs = self.PyBuiltIn.dict()
        j_kwargs = self.PyDict@j_kwargs
        j_kwargs.put("multiplier", 2)

        # Invoke via PyBuiltIn.call(PyCallable obj, PyTuple args, PyDict kwargs)
        j_result = self.PyBuiltIn.call(j_callable, j_args, j_kwargs)
        
        # (10 + 5) * 2 = 30
        self.assertEqual(int(j_result), 30)

