package python.lang;

import java.util.Arrays;
import org.jpype.bridge.Context;
import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

public class PyContainerNGTest
{
  private static Context context;

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    if (!Interpreter.getInstance().isStarted())
      Interpreter.getInstance().start(new String[0]);
    context = new Context();
  }

  @Test
  public void testListProxyImplementsPyContainer()
  {
    PyObject obj = PyBuiltIn.list(Arrays.asList("a", "b", "c"));

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("a"));
    assertFalse(((PyContainer<?>) obj).contains("z"));
  }

  @Test
  public void testTupleProxyImplementsPyContainer()
  {
    PyObject obj = PyBuiltIn.tuple("a", "b", "c");

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("a"));
    assertFalse(((PyContainer<?>) obj).contains("z"));
  }

  @Test
  public void testSetProxyImplementsPyContainer()
  {
    PyObject obj = PySet.of(Arrays.asList("a", "b", "c"));

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("a"));
    assertFalse(((PyContainer<?>) obj).contains("z"));
  }

  @Test
  public void testDictProxyImplementsPyContainerWithKeySemantics()
  {
    PyDict dict = PyBuiltIn.dict();
    dict.putAny("key1", "value1");
    dict.putAny("key2", "value2");

    PyObject obj = dict;

    assertTrue(obj instanceof PyContainer);

    PyContainer<?> container = (PyContainer<?>) obj;
    assertTrue(container.contains("key1"));
    assertTrue(container.contains("key2"));
    assertFalse(container.contains("value1"));
    assertFalse(container.contains("missing"));
  }

  @Test
  public void testPythonEvaluatedListImplementsPyContainer()
  {
    PyObject obj = (PyObject) context.eval("[1, 2, 3]");

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains(1));
    assertFalse(((PyContainer<?>) obj).contains(99));
  }

  @Test
  public void testPythonEvaluatedTupleImplementsPyContainer()
  {
    PyObject obj = (PyObject) context.eval("('x', 'y')");

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("x"));
    assertFalse(((PyContainer<?>) obj).contains("z"));
  }

  @Test
  public void testPythonEvaluatedDictImplementsPyContainerWithKeySemantics()
  {
    PyObject obj = (PyObject) context.eval("{'a': 1, 'b': 2}");

    assertTrue(obj instanceof PyContainer);

    PyContainer<?> container = (PyContainer<?>) obj;
    assertTrue(container.contains("a"));
    assertTrue(container.contains("b"));
    assertFalse(container.contains(1));
    assertFalse(container.contains("missing"));
  }

  @Test
  public void testEmptyContainerBehavior()
  {
    PyObject list = PyBuiltIn.list();
    PyObject tuple = PyBuiltIn.tuple();
    PyObject set = PyBuiltIn.set();
    PyObject dict = PyBuiltIn.dict();

    assertTrue(list instanceof PyContainer);
    assertTrue(tuple instanceof PyContainer);
    assertTrue(set instanceof PyContainer);
    assertTrue(dict instanceof PyContainer);

    assertFalse(((PyContainer<?>) list).contains("x"));
    assertFalse(((PyContainer<?>) tuple).contains("x"));
    assertFalse(((PyContainer<?>) set).contains("x"));
    assertFalse(((PyContainer<?>) dict).contains("x"));
  }
}