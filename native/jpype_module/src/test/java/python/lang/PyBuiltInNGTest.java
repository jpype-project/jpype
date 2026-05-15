package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.Arrays;
import org.jpype.bridge.Interpreter;

public class PyBuiltInNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    if (!Interpreter.getInstance().isStarted())
      Interpreter.getInstance().start(new String[0]);
  }

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
  public void testDict()
  {
    PyDict result = PyBuiltIn.dict();
    assertNotNull(result);
    assertEquals(PyBuiltIn.len(result), 0);
  }

  @Test
  public void testLen()
  {
    PyList list = PyBuiltIn.list(Arrays.asList(1, 2, 3));
    int result = PyBuiltIn.len(list);
    assertEquals(result, 3);
  }

  @Test
  public void testStr()
  {
    PyInt val = PyBuiltIn.$int(100);
    PyString result = PyBuiltIn.str(val);
    assertEquals(result.toString(), "100");
  }

  @Test
  public void testEval()
  {
    // Basic math eval
    PyObject result = PyBuiltIn.eval("1 + 1", PyBuiltIn.dict(), PyBuiltIn.dict());
    assertTrue(result instanceof PyInt);
    assertEquals(((PyInt) result).toNumber().longValue(), 2L);
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
  public void testGetattr()
  {
    PyString s = PyBuiltIn.str("hello");
    PyObject upperFunc = PyBuiltIn.getattr(s, "upper");
    assertTrue(upperFunc instanceof PyCallable);
  }

  @Test
  public void testRange_int()
  {
    PyRange result = PyBuiltIn.range(5);
    assertNotNull(result);
    assertEquals(PyBuiltIn.len(result), 5);
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
  public void testType()
  {
    PyInt val = PyBuiltIn.$int(10);
    PyType type = PyBuiltIn.type(val);
    assertEquals(type.toString(), "<class 'int'>");
  }

  @Test
  public void testRepr()
  {
    PyString s = PyBuiltIn.str("test");
    PyString result = PyBuiltIn.repr(s);
    // repr of string includes quotes
    assertEquals(result.toString(), "'test'");
  }
}
