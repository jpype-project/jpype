// --- file: python/lang/PyBuiltInNGTest.java ---
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import java.util.Arrays;
import java.util.List;

public class PyBuiltInNGTest extends PyTestHarness
{
  
  @Test
  public void test$float()
  {
    double value = 3.14;
    PyFloat result = PyBuiltIn.$float(value);
    assertNotNull(result);
    assertEquals(result.toNumber().doubleValue(), 3.14, 0.0001);
  }

  @Test
  public void test$int()
  {
    long value = 42L;
    PyInt result = PyBuiltIn.$int(value);
    assertNotNull(result);
    assertEquals(result.toNumber().longValue(), 42L);
  }

  @Test
  public void testBytesFromString()
  {
    PyBytes result = PyBuiltIn.bytes("abc");

    assertNotNull(result);
    assertEquals(result.size(), 3);
  }

  @Test
  public void testCall()
  {
    PyCallable callable = (PyCallable) context.eval("lambda x, y: x + y");
    PyTuple args = PyBuiltIn.tuple(2, 3);
    PyDict kwargs = PyBuiltIn.dict();

    PyObject result = PyBuiltIn.call(callable, args, kwargs);

    assertNotNull(result);
    assertEquals(result.toString(), "5");
  }

  @Test
  public void testDelattr()
  {
    context.exec(
            "class BuiltinAttrTest4:\n"
            + "    pass\n"
            + "obj_delattr = BuiltinAttrTest4()\n"
            + "obj_delattr.flag = 123\n");
    PyObject obj = (PyObject) context.eval("obj_delattr");

    assertTrue(PyBuiltIn.hasattr(obj, "flag"));
    PyBuiltIn.delattr(obj, "flag");
    assertFalse(PyBuiltIn.hasattr(obj, "flag"));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testDelattrMissingThrows()
  {
    context.exec(
            "class DelAttrMissing:\n"
            + "    pass\n"
            + "obj_missing_del = DelAttrMissing()\n");
    PyObject obj = (PyObject) context.eval("obj_missing_del");
    PyBuiltIn.delattr(obj, "does_not_exist");
  }

  @Test
  public void testDict()
  {
    PyDict result = PyBuiltIn.dict();
    assertNotNull(result);
    assertEquals(PyBuiltIn.len(result), 0);
  }

  @Test
  public void testDir()
  {
    PyString s = PyBuiltIn.str("hello");
    PyList result = PyBuiltIn.dir(s);

    assertNotNull(result);
    assertTrue(result.toString().contains("upper"));
  }

  @Test
  public void testEnumerateContents()
  {
    PyEnumerate result = PyBuiltIn.enumerate(Arrays.asList("a", "b"));
    PyList items = result.toList();

    assertEquals(items.size(), 2);
    assertTrue(items.get(0).toString().contains("0"));
    assertTrue(items.get(0).toString().contains("a"));
    assertTrue(items.get(1).toString().contains("1"));
    assertTrue(items.get(1).toString().contains("b"));
  }

  @Test
  public void testEnumerateIterable()
  {
    List<String> source = Arrays.asList("x", "y");
    PyEnumerate result = PyBuiltIn.enumerate(source);

    assertNotNull(result);
    PyList items = result.toList();
    assertEquals(items.size(), 2);
  }

  @Test
  public void testEnumeratePyObject()
  {
    PyList source = PyBuiltIn.list(Arrays.asList("x", "y"));
    PyEnumerate result = PyBuiltIn.enumerate((PyObject) source);

    assertNotNull(result);
    PyList items = result.toList();
    assertEquals(items.size(), 2);
  }

  @Test
  public void testEval()
  {
    // Basic math eval
    PyObject result = PyBuiltIn.eval("1 + 1", PyBuiltIn.dict(), PyBuiltIn.dict());
    assertTrue(result instanceof PyInt);
    assertEquals(((PyInt) result).toNumber().longValue(), 2L);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testEvalInvalidExpressionThrows()
  {
    PyBuiltIn.eval("1 + ", PyBuiltIn.dict(), PyBuiltIn.dict());
  }

  @Test
  public void testExec()
  {
    PyDict globals = PyBuiltIn.dict();
    PyDict locals = PyBuiltIn.dict();

    PyBuiltIn.exec("x = 123", globals, locals);

    assertTrue(locals.containsKey("x"));
    assertEquals(locals.get("x").toString(), "123");
  }

  @Test
  public void testGetattr()
  {
    PyString s = PyBuiltIn.str("hello");
    PyObject upperFunc = PyBuiltIn.getattr(s, "upper");
    assertTrue(upperFunc instanceof PyCallable);
  }

  @Test
  public void testGetattrDefaultExisting()
  {
    context.exec(
            "class BuiltinAttrTest2:\n"
            + "    pass\n"
            + "obj_existing = BuiltinAttrTest2()\n"
            + "obj_existing.name = 'alice'\n");
    PyObject obj = (PyObject) context.eval("obj_existing");
    PyObject fallback = PyString.from("fallback");

    PyObject result = PyBuiltIn.getattrDefault(obj, "name", fallback);

    assertNotNull(result);
    assertEquals(result.toString(), "alice");
  }

  @Test
  public void testGetattrDefaultMissing()
  {
    context.exec(
            "class BuiltinAttrTest:\n"
            + "    pass\n"
            + "obj_default = BuiltinAttrTest()\n");
    PyObject obj = (PyObject) context.eval("obj_default");
    PyObject fallback = PyString.from("fallback");
    PyObject result = PyBuiltIn.getattrDefault(obj, "missing", fallback);

    System.out.println(result.equals(fallback));
    System.out.println(fallback.equals(result));
    assertEquals(result, fallback);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testGetattrMissingThrows()
  {
    PyObject obj = (PyObject) context.eval("object()");
    PyBuiltIn.getattr(obj, "missing_attribute_name");
  }

  @Test
  public void testHasattr()
  {
    PyString s = PyBuiltIn.str("hello");
    // Strings in Python have upper()
    assertTrue(PyBuiltIn.hasattr(s, "upper"));
    assertFalse(PyBuiltIn.hasattr(s, "non_existent_method_123"));
  }

  @Test
  public void testIndices()
  {
    PySlice slice = PyBuiltIn.slice(1, 5, 2);
    PyTuple result = PyBuiltIn.indices(slice);

    assertNotNull(result);
    assertEquals(result.size(), 1);
  }

  @Test
  public void testIsinstanceFalse()
  {
    PyInt value = PyBuiltIn.$int(5);
    PyType stringType = PyBuiltIn.type(PyBuiltIn.str("abc"));

    assertFalse(PyBuiltIn.isinstance(value, stringType));
  }

  @Test
  public void testIsinstanceMultipleTypes()
  {
    PyObject value = PyBuiltIn.str("abc");
    PyType strType = PyBuiltIn.type(value);
    PyType intType = PyBuiltIn.type(PyBuiltIn.$int(1));

    assertTrue(PyBuiltIn.isinstance(value, intType, strType));
  }

  @Test
  public void testIsinstanceTrue()
  {
    PyInt value = PyBuiltIn.$int(5);
    PyType type = PyBuiltIn.type(value);

    assertTrue(PyBuiltIn.isinstance(value, type));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testIterOnNonIterableThrows()
  {
    PyBuiltIn.iter(PyBuiltIn.$int(5));
  }

  @Test
  public void testIterOnPyList()
  {
    PyList list = PyBuiltIn.list(Arrays.asList("a", "b"));
    PyIter<PyObject> iter = PyBuiltIn.iter(list);

    assertNotNull(iter);
    assertEquals(iter.next().toString(), "a");
    assertEquals(iter.next().toString(), "b");
  }

  @Test
  public void testLen()
  {
    PyList list = PyBuiltIn.list(Arrays.asList(1, 2, 3));
    int result = PyBuiltIn.len(list);
    assertEquals(result, 3);
  }

  @Test
  public void testListEmpty()
  {
    PyList result = PyBuiltIn.list();
    assertNotNull(result);
    assertEquals(result.size(), 0);
    assertTrue(result.isEmpty());
  }

  @Test
  public void testList_Object()
  {
    // Test conversion from Java List to Python List
    java.util.List<String> jList = Arrays.asList("a", "b");
    PyList pList = PyBuiltIn.list(jList);
    assertEquals(PyBuiltIn.len(pList), 2);
    assertEquals(pList.get(0).toString(), "a");
  }

  @Test
  public void testMemoryviewFromBytes()
  {
    PyBytes bytes = PyBuiltIn.bytes("abc");
    PyMemoryView result = PyBuiltIn.memoryview(bytes);

    assertNotNull(result);
  }

  @Test
  public void testNextWithStopDefault()
  {
    PyList list = PyBuiltIn.list(Arrays.asList("a"));
    PyIter<PyObject> iter = PyBuiltIn.iter(list);
    PyObject stop = PyString.from("stop");

    assertEquals(PyBuiltIn.next(iter, stop).toString(), "a");
    assertEquals(PyBuiltIn.next(iter, stop), stop);
  }

  @Test
  public void testRangeStartStop()
  {
    PyRange result = PyBuiltIn.range(2, 5);

    assertNotNull(result);
    assertEquals(result.getStart(), 2);
    assertEquals(result.getStop(), 5);
    assertEquals(result.getStep(), 1);
  }

  @Test
  public void testRangeStartStopStep()
  {
    PyRange result = PyBuiltIn.range(1, 7, 2);

    assertNotNull(result);
    assertEquals(result.getStart(), 1);
    assertEquals(result.getStop(), 7);
    assertEquals(result.getStep(), 2);
  }

  @Test
  public void testRange_int()
  {
    PyRange result = PyBuiltIn.range(5);
    assertNotNull(result);
    assertEquals(PyBuiltIn.len(result), 5);
  }

  @Test
  public void testRepr()
  {
    PyString s = PyBuiltIn.str("test");
    PyString result = PyBuiltIn.repr(s);
    // repr of string includes quotes
    assertEquals(result.toString(), "'test'");
  }

  @Test
  public void testReprList()
  {
    PyList list = PyBuiltIn.list(Arrays.asList("a", "b"));
    PyString result = PyBuiltIn.repr(list);

    assertNotNull(result);
    assertTrue(result.toString().contains("a"));
    assertTrue(result.toString().contains("b"));
  }

  @Test
  public void testSetEmpty()
  {
    PySet result = PyBuiltIn.set();
    assertNotNull(result);
    assertEquals(result.size(), 0);
    assertTrue(result.isEmpty());
  }

  @Test
  public void testSetattr()
  {
    context.exec(
            "class BuiltinAttrTest3:\n"
            + "    pass\n"
            + "obj_setattr = BuiltinAttrTest3()\n");
    PyObject obj = (PyObject) context.eval("obj_setattr");

    PyBuiltIn.setattr(obj, "city", "Paris");

    PyObject result = PyBuiltIn.getattr(obj, "city");
    assertEquals(result.toString(), "Paris");
  }

  @Test
  public void testSliceSingle()
  {
    PySlice result = PyBuiltIn.slice(3);

    assertNotNull(result);
    assertEquals(result.getStart(), Integer.valueOf(3));
    assertEquals(result.getStop(), Integer.valueOf(4));
  }

  @Test
  public void testSliceStartStop()
  {
    PySlice result = PyBuiltIn.slice(1, 5);

    assertNotNull(result);
    assertEquals(result.getStart(), Integer.valueOf(1));
    assertEquals(result.getStop(), Integer.valueOf(5));
    assertNull(result.getStep());
  }

  @Test
  public void testSliceStartStopStep()
  {
    PySlice result = PyBuiltIn.slice(1, 5, 2);

    assertNotNull(result);
    assertEquals(result.getStart(), Integer.valueOf(1));
    assertEquals(result.getStop(), Integer.valueOf(5));
    assertEquals(result.getStep(), Integer.valueOf(2));
  }

  @Test
  public void testStr()
  {
    PyInt val = PyBuiltIn.$int(100);
    PyString result = PyBuiltIn.str(val);
    assertEquals(result.toString(), "100");
  }

  @Test
  public void testStrList()
  {
    PyList list = PyBuiltIn.list(Arrays.asList("a", "b"));
    PyString result = PyBuiltIn.str(list);

    assertNotNull(result);
    assertTrue(result.toString().contains("a"));
    assertTrue(result.toString().contains("b"));
  }

  @Test
  public void testTupleEmpty()
  {
    PyTuple result = PyBuiltIn.tuple();
    assertNotNull(result);
    assertEquals(result.size(), 0);
    assertTrue(result.isEmpty());
  }

  @Test
  public void testTupleVarArgs()
  {
    PyTuple result = PyBuiltIn.tuple("a", "b", 3);
    assertNotNull(result);
    assertEquals(result.size(), 3);
    assertEquals(result.get(0).toString(), "a");
    assertEquals(result.get(1).toString(), "b");
    assertEquals(result.get(2).toString(), "3");
  }

  @Test
  public void testType()
  {
    PyInt val = PyBuiltIn.$int(10);
    PyType type = PyBuiltIn.type(val);
    assertEquals(type.toString(), "<class 'int'>");
  }

  @Test
  public void testVars()
  {
    context.exec(
            "class BuiltinVarsTest:\n"
            + "    pass\n"
            + "obj_vars = BuiltinVarsTest()\n"
            + "obj_vars.x = 10\n");
    PyObject obj = (PyObject) context.eval("obj_vars");

    PyDict result = PyBuiltIn.vars(obj);

    assertNotNull(result);
    assertTrue(result.containsKey("x"));
    assertEquals(result.get("x").toString(), "10");
  }

  @Test
  public void testZipBuiltIn()
  {
    PyZip result = PyBuiltIn.zip(Arrays.asList(1, 2), Arrays.asList("a", "b"));
    PyList items = result.toList();

    assertNotNull(items);
    assertEquals(items.size(), 2);
  }

}
