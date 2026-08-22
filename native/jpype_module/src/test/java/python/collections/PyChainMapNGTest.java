// --- file: python/collections/PyChainMapNGTest.java ---
package python.collections;

import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyDict;
import python.lang.PyList;
import python.lang.PyObject;
import python.lang.PyTestHarness;

public class PyChainMapNGTest extends PyTestHarness
{

  private PyDict dictOf(String k, int v)
  {
    PyDict d = context.dict();
    d.put(context.str(k), context.$int(v));
    return d;
  }

  @Test
  public void testCreateAndLookup()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    assertNotNull(cm);
    assertEquals(cm.get(context.str("a")).toString(), "1");
    assertEquals(cm.get(context.str("b")).toString(), "2");
    assertNull(cm.get(context.str("missing")));
  }

  @Test
  public void testWritesLandOnFirstMap()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    cm.put(context.str("c"), context.$int(3));

    assertEquals(cm.get(context.str("c")).toString(), "3");
    assertTrue(first.containsKey(context.str("c")));
    assertFalse(second.containsKey(context.str("c")));
  }

  @Test
  public void testShadowingFirstMapWins()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("a", 99);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    assertEquals(cm.get(context.str("a")).toString(), "1");
  }

  @Test
  public void testMaps()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    PyList maps = cm.maps();

    assertEquals(maps.size(), 2);
  }

  @Test
  public void testNewChildNoArg()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    PyChainMap child = cm.newChild();

    assertEquals(child.maps().size(), 2);
    assertEquals(child.get(context.str("a")).toString(), "1");

    child.put(context.str("x"), context.$int(42));
    assertFalse(first.containsKey(context.str("x")));
  }

  @Test
  public void testNewChildWithMap()
  {
    PyDict first = dictOf("a", 1);
    PyDict newFront = dictOf("z", 9);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    PyChainMap child = cm.newChild(newFront);

    assertEquals(child.get(context.str("z")).toString(), "9");
    assertEquals(child.get(context.str("a")).toString(), "1");
  }

  @Test
  public void testParents()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    PyChainMap parents = cm.parents();

    assertNull(parents.get(context.str("a")));
    assertEquals(parents.get(context.str("b")).toString(), "2");
  }

  @Test
  public void testContainsKeyAndSize()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    assertTrue(cm.containsKey(context.str("a")));
    assertTrue(cm.containsKey(context.str("b")));
    assertFalse(cm.containsKey(context.str("z")));
    assertEquals(cm.size(), 2);
  }

  // PyChainMap is the one concrete type in this codebase that doesn't
  // shadow PyMapping's own keySet()/values()/entrySet() defaults with a
  // type-specific override (unlike PyDict, which builds its own
  // PyDictKeySet/PyDictValues/PyDictItems) - so it's the real way to
  // reach PyMappingKeySet/PyMappingValues/PyMappingEntrySet(Iterator).

  @Test
  public void testKeySetContainsAndIteration()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    Set<PyObject> keys = cm.keySet();
    assertEquals(keys.size(), 2);
    assertFalse(keys.isEmpty());
    assertTrue(keys.contains(context.str("a")));
    assertTrue(keys.containsAll(Arrays.asList(context.str("a"), context.str("b"))));
    assertFalse(keys.containsAll(Arrays.asList(context.str("a"), context.str("z"))));

    java.util.HashSet<String> seen = new java.util.HashSet<>();
    for (PyObject k : keys)
      seen.add(k.toString());
    assertEquals(seen.size(), 2);
    assertTrue(seen.contains("a"));
    assertTrue(seen.contains("b"));

    Object[] arr = keys.toArray();
    assertEquals(arr.length, 2);
    Object[] arr2 = keys.toArray(new Object[0]);
    assertEquals(arr2.length, 2);
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testKeySetAddUnsupported()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    cm.keySet().add(context.str("z"));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testKeySetAddAllUnsupported()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    cm.keySet().addAll(Arrays.asList(context.str("z")));
  }

  @Test
  public void testKeySetRemoveAll()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    boolean changed = cm.keySet().removeAll(Arrays.asList(context.str("a")));

    assertTrue(changed);
    assertFalse(cm.containsKey(context.str("a")));
  }

  @Test
  public void testKeySetRetainAll()
  {
    PyDict first = dictOf("a", 1);
    first.put(context.str("b"), context.$int(2));
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    boolean changed = cm.keySet().retainAll(Arrays.asList(context.str("a")));

    assertTrue(changed);
    assertTrue(cm.containsKey(context.str("a")));
    assertFalse(cm.containsKey(context.str("b")));
  }

  @Test
  public void testKeySetRemove()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    assertTrue(cm.keySet().remove(context.str("a")));
    assertFalse(cm.containsKey(context.str("a")));
  }

  @Test
  public void testKeySetClear()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    cm.keySet().clear();
    assertTrue(cm.isEmpty());
  }

  @Test
  public void testValuesContainsAndIteration()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    Collection<PyObject> values = cm.values();
    assertEquals(values.size(), 2);
    assertFalse(values.isEmpty());
    assertTrue(values.contains(context.$int(1)));
    assertTrue(values.containsAll(Arrays.asList(context.$int(1), context.$int(2))));

    java.util.HashSet<String> seen = new java.util.HashSet<>();
    for (PyObject v : values)
      seen.add(v.toString());
    assertEquals(seen.size(), 2);
    assertTrue(seen.contains("1"));
    assertTrue(seen.contains("2"));

    assertEquals(values.toArray().length, 2);
    assertEquals(values.toArray(new Object[0]).length, 2);
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testValuesAddUnsupported()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    cm.values().add(context.$int(9));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testValuesAddAllUnsupported()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    cm.values().addAll(Arrays.asList(context.$int(9)));
  }

  @Test
  public void testValuesRemove()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    boolean changed = cm.values().remove(context.$int(1));

    assertTrue(changed);
    assertFalse(cm.containsKey(context.str("a")));
  }

  @Test
  public void testValuesRemoveAll()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));

    boolean changed = cm.values().removeAll(Arrays.asList(context.$int(1)));

    assertTrue(changed);
    assertFalse(cm.containsKey(context.str("a")));
  }

  @Test
  public void testValuesRetainAll()
  {
    PyDict first = dictOf("a", 1);
    first.put(context.str("b"), context.$int(2));
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    boolean changed = cm.values().retainAll(Arrays.asList(context.$int(1)));

    assertTrue(changed);
    assertTrue(cm.containsKey(context.str("a")));
    assertFalse(cm.containsKey(context.str("b")));
  }

  @Test
  public void testValuesClear()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    cm.values().clear();
    assertTrue(cm.isEmpty());
  }

  @Test
  public void testEntrySetIterationAndSetValue()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));

    Set<Map.Entry<PyObject, PyObject>> entries = cm.entrySet();
    assertEquals(entries.size(), 1);
    assertFalse(entries.isEmpty());

    Iterator<Map.Entry<PyObject, PyObject>> it = entries.iterator();
    assertTrue(it.hasNext());
    Map.Entry<PyObject, PyObject> entry = it.next();
    assertEquals(entry.getKey().toString(), "a");
    assertEquals(entry.getValue().toString(), "1");

    PyObject previous = entry.setValue(context.$int(42));
    assertEquals(previous.toString(), "1");
    assertEquals(cm.get(context.str("a")).toString(), "42");

    assertFalse(it.hasNext());
    try
    {
      it.next();
      fail("Expected NoSuchElementException");
    } catch (java.util.NoSuchElementException ex)
    {
      // expected
    }
  }

  @Test
  public void testEntrySetContainsAndRemoveAlwaysFalse()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    Set<Map.Entry<PyObject, PyObject>> entries = cm.entrySet();

    assertFalse(entries.contains("anything"));
    assertFalse(entries.containsAll(Arrays.asList("anything")));
    assertFalse(entries.remove("anything"));
    assertFalse(entries.removeAll(Arrays.asList("anything")));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testEntrySetRetainAllThrows()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    cm.entrySet().retainAll(Arrays.asList());
  }

  @Test
  public void testEntrySetAddAndClear()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    Set<Map.Entry<PyObject, PyObject>> entries = cm.entrySet();

    boolean added = entries.add(new java.util.AbstractMap.SimpleEntry<>(context.str("z"), context.$int(9)));
    assertTrue(added);
    assertEquals(cm.get(context.str("z")).toString(), "9");

    // Re-adding the same key updates the value and reports false (already present).
    boolean addedAgain = entries.add(new java.util.AbstractMap.SimpleEntry<>(context.str("z"), context.$int(10)));
    assertFalse(addedAgain);
    assertEquals(cm.get(context.str("z")).toString(), "10");

    entries.clear();
    assertTrue(cm.isEmpty());
  }

  @Test
  public void testEntrySetAddAll()
  {
    PyDict first = dictOf("a", 1);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first));
    Set<Map.Entry<PyObject, PyObject>> entries = cm.entrySet();

    java.util.List<Map.Entry<PyObject, PyObject>> toAdd = Arrays.asList(
            new java.util.AbstractMap.SimpleEntry<>(context.str("y"), context.$int(8)),
            new java.util.AbstractMap.SimpleEntry<>(context.str("z"), context.$int(9)));

    boolean result = entries.addAll(toAdd);

    assertTrue(result);
    assertEquals(cm.get(context.str("y")).toString(), "8");
    assertEquals(cm.get(context.str("z")).toString(), "9");
    assertEquals(cm.size(), 3);
  }

  @Test
  public void testEntrySetToArray()
  {
    PyDict first = dictOf("a", 1);
    PyDict second = dictOf("b", 2);
    PyChainMap cm = PyCollections.using(context).chainMap(Arrays.asList(first, second));
    Set<Map.Entry<PyObject, PyObject>> entries = cm.entrySet();

    assertEquals(entries.toArray().length, 2);
    assertEquals(entries.toArray(new Object[0]).length, 2);
  }
}
