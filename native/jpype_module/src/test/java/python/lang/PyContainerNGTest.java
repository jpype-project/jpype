// --- file: python/lang/PyContainerNGTest.java ---
package python.lang;

import java.util.Arrays;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyContainerNGTest extends PyTestHarness
{

  @Test
  public void testDictProxyImplementsPyContainerWithKeySemantics()
  {
    PyDict dict = context.dict();
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
  public void testEmptyContainerBehavior()
  {
    PyObject list = context.list();
    PyObject tuple = context.tuple();
    PyObject set = context.set();
    PyObject dict = context.dict();

    assertTrue(list instanceof PyContainer);
    assertTrue(tuple instanceof PyContainer);
    assertTrue(set instanceof PyContainer);
    assertTrue(dict instanceof PyContainer);

    assertFalse(((PyContainer<?>) list).contains("x"));
    assertFalse(((PyContainer<?>) tuple).contains("x"));
    assertFalse(((PyContainer<?>) set).contains("x"));
    assertFalse(((PyContainer<?>) dict).contains("x"));
  }

  @Test
  public void testListProxyImplementsPyContainer()
  {
    PyObject obj = context.list(Arrays.asList("a", "b", "c"));

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("a"));
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
  public void testSetProxyImplementsPyContainer()
  {
    PyObject obj = context.set(Arrays.asList("a", "b", "c"));

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("a"));
    assertFalse(((PyContainer<?>) obj).contains("z"));
  }

  @Test
  public void testTupleProxyImplementsPyContainer()
  {
    PyObject obj = context.tuple("a", "b", "c");

    assertTrue(obj instanceof PyContainer);
    assertTrue(((PyContainer<?>) obj).contains("a"));
    assertFalse(((PyContainer<?>) obj).contains("z"));
  }
}
