// --- file: python/lang/PyDictKeySetNGTest.java ---
package python.lang;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyDictKeySetNGTest extends PyTestHarness
{


  private PyDict dictOf(Object... items)
  {
    PyDict dict = context.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddAllUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<Object> keys = new PyDictKeySet<>(dict);

    keys.addAll(Arrays.asList("b", "c"));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testAddUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<Object> keys = new PyDictKeySet<>(dict);

    keys.add("b");
  }

  @Test
  public void testClearClearsUnderlyingDict()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    keys.clear();

    assertTrue(keys.isEmpty());
    assertTrue(dict.isEmpty());
    assertEquals(dict.size(), 0);
  }

  @Test
  public void testContainsAll()
  {
    PyDict dict = dictOf("a", 1, "b", 2, "c", 3);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertTrue(keys.containsAll(Arrays.asList("a", "b")));
    assertFalse(keys.containsAll(Arrays.asList("a", "z")));
  }

  @Test
  public void testContainsExistingKey()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertTrue(keys.contains("a"));
    assertTrue(keys.contains("b"));
  }

  @Test
  public void testContainsMissingKey()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertFalse(keys.contains("z"));
  }

  @Test
  public void testIsEmptyFalse()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertFalse(keys.isEmpty());
  }

  @Test
  public void testIsEmptyTrue()
  {
    PyDict dict = context.dict();
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertTrue(keys.isEmpty());
  }

  @Test
  public void testIteratorYieldsAllKeys()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    Set<String> out = new HashSet<>();
    Iterator<?> it = keys.iterator();
    while (it.hasNext())
      out.add(it.next().toString());

    assertEquals(out.size(), 2);
    assertTrue(out.contains("a"));
    assertTrue(out.contains("b"));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveAllUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    keys.removeAll(Arrays.asList("a"));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    keys.remove("a");
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRetainAllUnsupported()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    keys.retainAll(Arrays.asList("a"));
  }

  @Test
  public void testSize()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertEquals(keys.size(), 2);
  }

  @Test
  public void testToArray()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    Object[] array = keys.toArray();

    assertNotNull(array);
    assertEquals(array.length, 2);
  }

  @Test
  public void testToTypedArray()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    Object[] array = keys.toArray(new Object[0]);

    assertNotNull(array);
    assertEquals(array.length, 2);
  }

  @Test
  public void testViewReflectsDictMutation()
  {
    PyDict dict = dictOf("a", 1);
    PyDictKeySet<?> keys = new PyDictKeySet<>(dict);

    assertTrue(keys.contains("a"));
    assertFalse(keys.contains("b"));

    dict.putAny("b", 2);
    assertTrue(keys.contains("b"));

    dict.remove("a");
    assertFalse(keys.contains("a"));
  }

}
