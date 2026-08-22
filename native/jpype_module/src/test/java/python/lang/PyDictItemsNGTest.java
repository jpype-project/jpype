// --- file: python/lang/PyDictItemsNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package python.lang;

import java.util.AbstractMap;
import java.util.Iterator;
import java.util.Map;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyDictItemsNGTest extends PyTestHarness
{

  private PyDict dictOf(Object... items)
  {
    PyDict dict = context.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @Test
  public void testSizeReflectsDict()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictItems items = new PyDictItems(dict);
    assertEquals(items.size(), 2);
  }

  @Test
  public void testIsEmpty()
  {
    PyDictItems items = new PyDictItems(context.dict());
    assertTrue(items.isEmpty());
  }

  @Test
  public void testIsEmptyFalse()
  {
    PyDictItems items = new PyDictItems(dictOf("a", 1));
    assertFalse(items.isEmpty());
  }

  @Test
  public void testContains()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItems items = new PyDictItems(dict);
    Map.Entry<PyObject, PyObject> entry = new AbstractMap.SimpleEntry<>(context.str("a"), context.$int(1));
    assertTrue(items.contains(entry));
  }

  @Test
  public void testIteratorYieldsAllEntries()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictItems items = new PyDictItems(dict);

    int count = 0;
    Iterator<Map.Entry<PyObject, PyObject>> it = items.iterator();
    while (it.hasNext())
    {
      Map.Entry<PyObject, PyObject> entry = it.next();
      assertNotNull(entry.getKey());
      assertNotNull(entry.getValue());
      count++;
    }
    assertEquals(count, 2);
  }

  @Test
  public void testClearClearsUnderlyingDict()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictItems items = new PyDictItems(dict);

    items.clear();

    assertTrue(items.isEmpty());
    assertTrue(dict.isEmpty());
  }

  @Test
  public void testAdd()
  {
    PyDict dict = context.dict();
    PyDictItems items = new PyDictItems(dict);

    items.add(new AbstractMap.SimpleEntry<>(context.str("a"), context.$int(1)));

    assertEquals(dict.size(), 1);
  }

  @Test
  public void testAddAll()
  {
    PyDict dict = context.dict();
    PyDictItems items = new PyDictItems(dict);

    java.util.List<Map.Entry<PyObject, PyObject>> toAdd = java.util.Arrays.asList(
            new AbstractMap.SimpleEntry<>(context.str("a"), context.$int(1)),
            new AbstractMap.SimpleEntry<>(context.str("b"), context.$int(2)));

    boolean changed = items.addAll(toAdd);

    assertTrue(changed);
    assertEquals(dict.size(), 2);
    assertEquals(dict.get(context.str("a")).toString(), "1");
    assertEquals(dict.get(context.str("b")).toString(), "2");
  }

  @Test
  public void testAddAllReportsNoChangeWhenValuesIdentical()
  {
    // Re-add using the *same* PyObject value instance already in the dict,
    // not just an equal-looking freshly-constructed one - PyObject.equals()
    // isn't guaranteed to do cross-instance Python `__eq__` value comparison
    // (no other test in this suite relies on that), so pinning down "no
    // change" unambiguously requires the identical reference round-tripping
    // back out of putAny().
    PyObject one = context.$int(1);
    PyDict dict = context.dict();
    dict.putAny("a", one);
    PyDictItems items = new PyDictItems(dict);

    java.util.List<Map.Entry<PyObject, PyObject>> toAdd = java.util.Arrays.asList(
            new AbstractMap.SimpleEntry<>(context.str("a"), one));

    boolean changed = items.addAll(toAdd);

    assertFalse(changed);
    assertEquals(dict.size(), 1);
  }

  @Test
  public void testContainsAllTrue()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    PyDictItems items = new PyDictItems(dict);

    java.util.List<Map.Entry<PyObject, PyObject>> toCheck = java.util.Arrays.asList(
            new AbstractMap.SimpleEntry<>(context.str("a"), context.$int(1)),
            new AbstractMap.SimpleEntry<>(context.str("b"), context.$int(2)));

    assertTrue(items.containsAll(toCheck));
  }

  @Test
  public void testContainsAllFalseWhenOneEntryMissing()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItems items = new PyDictItems(dict);

    java.util.List<Map.Entry<PyObject, PyObject>> toCheck = java.util.Arrays.asList(
            new AbstractMap.SimpleEntry<>(context.str("a"), context.$int(1)),
            new AbstractMap.SimpleEntry<>(context.str("missing"), context.$int(99)));

    assertFalse(items.containsAll(toCheck));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveUnsupported()
  {
    PyDictItems items = new PyDictItems(dictOf("a", 1));
    items.remove(new AbstractMap.SimpleEntry<>(context.str("a"), context.$int(1)));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveAllUnsupported()
  {
    PyDictItems items = new PyDictItems(dictOf("a", 1));
    items.removeAll(java.util.Collections.emptyList());
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRetainAllUnsupported()
  {
    PyDictItems items = new PyDictItems(dictOf("a", 1));
    items.retainAll(java.util.Collections.emptyList());
  }

  @Test
  public void testToArray()
  {
    PyDictItems items = new PyDictItems(dictOf("a", 1, "b", 2));
    Object[] array = items.toArray();
    assertEquals(array.length, 2);
  }

  @Test
  public void testToTypedArray()
  {
    PyDictItems items = new PyDictItems(dictOf("a", 1, "b", 2));
    Object[] array = items.toArray(new Object[0]);
    assertEquals(array.length, 2);
  }

  @Test
  public void testViewReflectsDictMutation()
  {
    PyDict dict = dictOf("a", 1);
    PyDictItems items = new PyDictItems(dict);

    assertEquals(items.size(), 1);
    dict.putAny("b", 2);
    assertEquals(items.size(), 2);
  }

}
