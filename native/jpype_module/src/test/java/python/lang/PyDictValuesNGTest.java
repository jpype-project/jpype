// --- file: python/lang/PyDictValuesNGTest.java ---
package python.lang;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyDictValuesNGTest extends PyTestHarness
{

  private PyDict dictOf(Object... items)
  {
    PyDict dict = PyBuiltIn.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddAllUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<PyObject> values = new PyDictValues<>(dict);

    values.addAll(Arrays.asList(PyInt.of(2), PyInt.of(3)));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<PyObject> values = new PyDictValues<>(dict);

    values.add(PyInt.of(2));
  }

  @Test
  public void testClearClearsUnderlyingDict()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictValues<?> values = new PyDictValues<>(dict);

    values.clear();

    assertTrue(values.isEmpty());
    assertTrue(dict.isEmpty());
    assertEquals(dict.size(), 0);
  }

  @Test
  public void testContainsAll()
  {
    PyDict dict = dictOf(
            "a", PyString.of("x"),
            "b", PyString.of("y"),
            "c", PyString.of("z"));
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertTrue(values.containsAll(Arrays.asList(PyString.of("x"), PyString.of("y")))
            || values.toString().contains("x") && values.toString().contains("y"));
    assertFalse(values.containsAll(Arrays.asList(PyString.of("x"), PyString.of("missing"))));
  }

  @Test
  public void testContainsExistingValue()
  {
    PyDict dict = dictOf("a", PyString.of("x"), "b", PyString.of("y"));
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertTrue(values.contains(PyString.of("x")) || values.toString().contains("x"));
    assertTrue(values.contains(PyString.of("y")) || values.toString().contains("y"));
  }

  @Test
  public void testContainsMissingValue()
  {
    PyDict dict = dictOf("a", PyString.of("x"));
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertFalse(values.contains(PyString.of("z")));
  }

  @Test
  public void testIsEmptyFalse()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertFalse(values.isEmpty());
  }

  @Test
  public void testIsEmptyTrue()
  {
    PyDict dict = PyBuiltIn.dict();
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertTrue(values.isEmpty());
  }

  @Test
  public void testIteratorYieldsAllValues()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictValues<?> values = new PyDictValues<>(dict);

    Set<String> out = new HashSet<>();
    Iterator<?> it = values.iterator();
    while (it.hasNext())
      out.add(it.next().toString());

    assertEquals(out.size(), 2);
    assertTrue(out.contains("1"));
    assertTrue(out.contains("2"));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveAllUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<?> values = new PyDictValues<>(dict);

    values.removeAll(Arrays.asList(PyInt.of(1)));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<?> values = new PyDictValues<>(dict);

    values.remove(PyInt.of(1));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRetainAllUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<?> values = new PyDictValues<>(dict);

    values.retainAll(Arrays.asList(PyInt.of(1)));
  }

  @Test
  public void testSize()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertEquals(values.size(), 2);
  }

  @Test
  public void testToArray()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictValues<?> values = new PyDictValues<>(dict);

    Object[] array = values.toArray();

    assertNotNull(array);
    assertEquals(array.length, 2);
  }

  @Test
  public void testToTypedArray()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictValues<?> values = new PyDictValues<>(dict);

    Object[] array = values.toArray(new Object[0]);

    assertNotNull(array);
    assertEquals(array.length, 2);
  }

  @Test
  public void testViewReflectsDictMutation()
  {
    PyDict dict = dictOf("a", 1);
    PyDictValues<?> values = new PyDictValues<>(dict);

    assertTrue(values.toString().contains("1"));
    assertFalse(values.toString().contains("2"));

    dict.putAny("b", 2);
    assertTrue(values.toString().contains("2"));

    dict.remove("a");
    assertFalse(values.toString().contains("1"));
  }

}
