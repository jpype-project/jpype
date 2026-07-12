// --- file: python/lang/PyDictItemsIteratorNGTest.java ---
package python.lang;

import java.util.Map;
import java.util.NoSuchElementException;
import java.util.function.BiFunction;
import org.jpype.MainInterpreter;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyDictItemsIteratorNGTest extends PyTestHarness
{


  private PyDict dictOf(Object... items)
  {
    PyDict dict = context.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(toPyObject(items[i]), toPyObject(items[i + 1]));
    return dict;
  }

  // Raw Java values (String, Integer, ...) never become native Python
  // objects when passed through the bridge (convertStrings defaults to
  // false), so round-tripping them through the dict (e.g. via an iterator's
  // setValue()) hits mismatched wrapper types on the way back. Build real
  // PyObjects up front instead.
  private Object toPyObject(Object value)
  {
    if (value instanceof String)
      return context.str((String) value);
    if (value instanceof Integer)
      return context.$int((Integer) value);
    return value;
  }

  @SuppressWarnings("unchecked")
  private PyDictItemsIterator<PyObject, PyObject> newIterator(PyDict dict)
  {
    PyObject itemsView = MainInterpreter.getInstance().getBackend().items(dict);
    PyIter<PyTuple> iter = context.<PyTuple>iter(itemsView);
    BiFunction<PyObject, PyObject, PyObject> setter = dict::put;
    return new PyDictItemsIterator<>(iter, setter);
  }

  @Test
  public void testEmptyIteratorHasNoNext()
  {
    PyDict dict = context.dict();
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    assertFalse(iterator.hasNext());
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testEmptyIteratorNextThrows()
  {
    PyDict dict = context.dict();
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    iterator.next();
  }

  @Test
  public void testEntrySetValueUpdatesUnderlyingDict()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItemsIterator<PyObject, PyObject> iterator = newIterator(dict);

    Map.Entry<PyObject, PyObject> entry = iterator.next();
    PyObject old = entry.setValue(context.$int(42));

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
