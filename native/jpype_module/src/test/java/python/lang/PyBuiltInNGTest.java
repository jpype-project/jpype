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
    PyFloat result = context.$float(value);
    assertNotNull(result);
    assertEquals(result.toNumber().doubleValue(), 3.14, 0.0001);
  }

  @Test
  public void test$int()
  {
    long value = 42L;
    PyInt result = context.$int(value);
    assertNotNull(result);
    assertEquals(result.toNumber().longValue(), 42L);
  }

  @Test
  public void testBytesFromString()
  {
    PyBytes result = context.bytes("abc");

    assertNotNull(result);
    assertEquals(result.size(), 3);
  }

  @Test
  public void testCall()
  {
    PyCallable callable = (PyCallable) context.eval("lambda x, y: x + y");
    PyTuple args = context.tuple(2, 3);
    PyDict kwargs = context.dict();

    PyObject result = context.call(callable, args, kwargs);

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

    assertTrue(context.hasattr(obj, "flag"));
    context.delattr(obj, "flag");
    assertFalse(context.hasattr(obj, "flag"));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testDelattrMissingThrows()
  {
    context.exec(
            "class DelAttrMissing:\n"
            + "    pass\n"
            + "obj_missing_del = DelAttrMissing()\n");
    PyObject obj = (PyObject) context.eval("obj_missing_del");
    context.delattr(obj, "does_not_exist");
  }

  @Test
  public void testDict()
  {
    PyDict result = context.dict();
    assertNotNull(result);
    assertEquals(context.len(result), 0);
  }

  @Test
  public void testDir()
  {
    PyString s = context.str("hello");
    PyList result = context.dir(s);

    assertNotNull(result);
    assertTrue(result.toString().contains("upper"));
  }

  @Test
  public void testEnumerateContents()
  {
    PyEnumerate result = context.enumerate(Arrays.asList("a", "b"));
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
    PyEnumerate result = context.enumerate(source);

    assertNotNull(result);
    PyList items = result.toList();
    assertEquals(items.size(), 2);
  }

  @Test
  public void testEnumeratePyObject()
  {
    PyList source = context.list(Arrays.asList("x", "y"));
    PyEnumerate result = context.enumerate((PyObject) source);

    assertNotNull(result);
    PyList items = result.toList();
    assertEquals(items.size(), 2);
  }

  @Test
  public void testEval()
  {
    // Basic math eval
    PyObject result = context.eval("1 + 1", context.dict(), context.dict());
    assertTrue(result instanceof PyInt);
    assertEquals(((PyInt) result).toNumber().longValue(), 2L);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testEvalInvalidExpressionThrows()
  {
    context.eval("1 + ", context.dict(), context.dict());
  }

  @Test
  public void testExec()
  {
    PyDict globals = context.dict();
    PyDict locals = context.dict();

    context.exec("x = 123", globals, locals);

    assertTrue(locals.containsKey("x"));
    assertEquals(locals.get("x").toString(), "123");
  }

  @Test
  public void testGetattr()
  {
    PyString s = context.str("hello");
    PyObject upperFunc = context.getattr(s, "upper");
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
    PyObject fallback = context.str("fallback");

    PyObject result = context.getattrDefault(obj, "name", fallback);

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
    PyObject fallback = context.str("fallback");
    PyObject result = context.getattrDefault(obj, "missing", fallback);

    System.out.println(result.equals(fallback));
    System.out.println(fallback.equals(result));
    assertEquals(result, fallback);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testGetattrMissingThrows()
  {
    PyObject obj = (PyObject) context.eval("object()");
    context.getattr(obj, "missing_attribute_name");
  }

  @Test
  public void testHasattr()
  {
    PyString s = context.str("hello");
    // Strings in Python have upper()
    assertTrue(context.hasattr(s, "upper"));
    assertFalse(context.hasattr(s, "non_existent_method_123"));
  }

  @Test
  public void testIndices()
  {
    PySlice slice = context.slice(1, 5, 2);
    PyTuple result = context.indices(slice);

    assertNotNull(result);
    assertEquals(result.size(), 1);
  }

  @Test
  public void testIsinstanceFalse()
  {
    PyInt value = context.$int(5);
    PyType stringType = context.type(context.str("abc"));

    assertFalse(context.isinstance(value, stringType));
  }

  @Test
  public void testIsinstanceMultipleTypes()
  {
    PyObject value = context.str("abc");
    PyType strType = context.type(value);
    PyType intType = context.type(context.$int(1));

    assertTrue(context.isinstance(value, intType, strType));
  }

  @Test
  public void testIsinstanceTrue()
  {
    PyInt value = context.$int(5);
    PyType type = context.type(value);

    assertTrue(context.isinstance(value, type));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testIterOnNonIterableThrows()
  {
    context.iter(context.$int(5));
  }

  @Test
  public void testIterOnPyList()
  {
    PyList list = context.list(Arrays.asList("a", "b"));
    PyIter<PyObject> iter = context.iter(list);

    assertNotNull(iter);
    assertEquals(iter.next().toString(), "a");
    assertEquals(iter.next().toString(), "b");
  }

  @Test
  public void testLen()
  {
    PyList list = context.list(Arrays.asList(1, 2, 3));
    int result = context.len(list);
    assertEquals(result, 3);
  }

  @Test
  public void testListEmpty()
  {
    PyList result = context.list();
    assertNotNull(result);
    assertEquals(result.size(), 0);
    assertTrue(result.isEmpty());
  }

  @Test
  public void testList_Object()
  {
    // Test conversion from Java List to Python List
    java.util.List<String> jList = Arrays.asList("a", "b");
    PyList pList = context.list(jList);
    assertEquals(context.len(pList), 2);
    assertEquals(pList.get(0).toString(), "a");
  }

  @Test
  public void testMemoryviewFromBytes()
  {
    PyBytes bytes = context.bytes("abc");
    PyMemoryView result = context.memoryview(bytes);

    assertNotNull(result);
  }

  @Test
  public void testNextWithStopDefault()
  {
    PyList list = context.list(Arrays.asList("a"));
    PyIter<PyObject> iter = context.iter(list);
    PyObject stop = context.str("stop");

    assertEquals(context.next(iter, stop).toString(), "a");
    assertEquals(context.next(iter, stop), stop);
  }

  @Test
  public void testRangeStartStop()
  {
    PyRange result = context.range(2, 5);

    assertNotNull(result);
    assertEquals(result.getStart(), 2);
    assertEquals(result.getStop(), 5);
    assertEquals(result.getStep(), 1);
  }

  @Test
  public void testRangeStartStopStep()
  {
    PyRange result = context.range(1, 7, 2);

    assertNotNull(result);
    assertEquals(result.getStart(), 1);
    assertEquals(result.getStop(), 7);
    assertEquals(result.getStep(), 2);
  }

  @Test
  public void testRange_int()
  {
    PyRange result = context.range(5);
    assertNotNull(result);
    assertEquals(context.len(result), 5);
  }

  @Test
  public void testRepr()
  {
    PyString s = context.str("test");
    PyString result = context.repr(s);
    // repr of string includes quotes
    assertEquals(result.toString(), "'test'");
  }

  @Test
  public void testReprList()
  {
    PyList list = context.list(Arrays.asList("a", "b"));
    PyString result = context.repr(list);

    assertNotNull(result);
    assertTrue(result.toString().contains("a"));
    assertTrue(result.toString().contains("b"));
  }

  @Test
  public void testSetEmpty()
  {
    PySet result = context.set();
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

    context.setattr(obj, "city", "Paris");

    PyObject result = context.getattr(obj, "city");
    assertEquals(result.toString(), "Paris");
  }

  @Test
  public void testSliceSingle()
  {
    PySlice result = context.slice(3);

    assertNotNull(result);
    assertEquals(result.getStart(), Integer.valueOf(3));
    assertEquals(result.getStop(), Integer.valueOf(4));
  }

  @Test
  public void testSliceStartStop()
  {
    PySlice result = context.slice(1, 5);

    assertNotNull(result);
    assertEquals(result.getStart(), Integer.valueOf(1));
    assertEquals(result.getStop(), Integer.valueOf(5));
    assertNull(result.getStep());
  }

  @Test
  public void testSliceStartStopStep()
  {
    PySlice result = context.slice(1, 5, 2);

    assertNotNull(result);
    assertEquals(result.getStart(), Integer.valueOf(1));
    assertEquals(result.getStop(), Integer.valueOf(5));
    assertEquals(result.getStep(), Integer.valueOf(2));
  }

  @Test
  public void testStr()
  {
    PyInt val = context.$int(100);
    PyString result = context.str(val);
    assertEquals(result.toString(), "100");
  }

  @Test
  public void testStrList()
  {
    PyList list = context.list(Arrays.asList("a", "b"));
    PyString result = context.str(list);

    assertNotNull(result);
    assertTrue(result.toString().contains("a"));
    assertTrue(result.toString().contains("b"));
  }

  @Test
  public void testTupleEmpty()
  {
    PyTuple result = context.tuple();
    assertNotNull(result);
    assertEquals(result.size(), 0);
    assertTrue(result.isEmpty());
  }

  @Test
  public void testTupleVarArgs()
  {
    PyTuple result = context.tuple("a", "b", 3);
    assertNotNull(result);
    assertEquals(result.size(), 3);
    assertEquals(result.get(0).toString(), "a");
    assertEquals(result.get(1).toString(), "b");
    assertEquals(result.get(2).toString(), "3");
  }

  @Test
  public void testType()
  {
    PyInt val = context.$int(10);
    PyType type = context.type(val);
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

    PyDict result = context.vars(obj);

    assertNotNull(result);
    assertTrue(result.containsKey("x"));
    assertEquals(result.get("x").toString(), "10");
  }

  @Test
  public void testZipBuiltIn()
  {
    PyZip result = context.zip(Arrays.asList(1, 2), Arrays.asList("a", "b"));
    PyList items = result.toList();

    assertNotNull(items);
    assertEquals(items.size(), 2);
  }

}
