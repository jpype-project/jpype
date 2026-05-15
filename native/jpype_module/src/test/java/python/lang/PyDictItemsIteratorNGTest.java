// --- file: python/lang/PyDictItemsIteratorNGTest.java ---
package python.lang;

import java.util.Map;
import java.util.NoSuchElementException;
import java.util.function.BiFunction;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyDictItemsIteratorNGTest extends PyTestHarness
{


  private PyDict dictOf(Object... items)
  {
    PyDict dict = PyBuiltIn.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @SuppressWarnings("unchecked")
  private PyDictItemsIterator<PyObject, PyObject> newIterator(PyDict dict)
  {
    PyObject itemsView = PyBuiltIn.backend().items(dict);
    PyIter<PyTuple> iter = PyBuiltIn.<PyTuple>iter(itemsView);
    BiFunction<PyObject, PyObject, PyObject> setter = dict::put;
    return new PyDictItemsIterator<>(iter, setter);
  }

  @Test
  public void testEmptyIteratorHasNoNext()
  {
    PyDict dict = PyBuiltIn.dict();
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    assertFalse(iterator.hasNext());
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testEmptyIteratorNextThrows()
  {
    PyDict dict = PyBuiltIn.dict();
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    iterator.next();
  }

  @Test
  public void testEntrySetValueUpdatesUnderlyingDict()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    Map.Entry<PyObject, PyObject> entry = iterator.next();
    PyObject old = entry.setValue(PyInt.of(42));

    assertEquals(old.toString(), "1");
    assertEquals(dict.get("a").toString(), "42");
  }

  @Test
  public void testHasNextFalseAfterExhaustion()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    assertTrue(iterator.hasNext());
    iterator.next();
    assertFalse(iterator.hasNext());
  }

  @Test
  public void testHasNextOnNonEmptyIterator()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    assertTrue(iterator.hasNext());
  }

  @Test
  public void testHasNextRepeatedDoesNotAdvance()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    assertTrue(iterator.hasNext());
    assertTrue(iterator.hasNext());

    Map.Entry<PyObject, PyObject> entry = iterator.next();
    assertEquals(entry.getKey().toString(), "a");
    assertEquals(entry.getValue().toString(), "1");
  }

  @Test
  public void testNextReturnsEntries()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    int count = 0;
    boolean sawA = false;
    boolean sawB = false;

    while (iterator.hasNext())
    {
      Map.Entry<PyObject, PyObject> entry = iterator.next();
      String key = entry.getKey().toString();
      String value = entry.getValue().toString();

      if (key.equals("a") && value.equals("1"))
        sawA = true;
      if (key.equals("b") && value.equals("2"))
        sawB = true;

      count++;
    }

    assertEquals(count, 2);
    assertTrue(sawA);
    assertTrue(sawB);
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testNextThrowsWhenExhausted()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    iterator.next();
    iterator.next();
  }

}
