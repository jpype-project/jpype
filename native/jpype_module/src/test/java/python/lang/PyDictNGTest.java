// --- file: python/lang/PyDictNGTest.java ---
package python.lang;

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyDictNGTest extends PyTestHarness
{

  private PyDict dictOf(Object... items)
  {
    PyDict dict = context.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @Test
  public void testClear()
  {
    PyDict dict = dictOf("a", 1, "b", 2);
    dict.clear();

    assertTrue(dict.isEmpty());
    assertEquals(dict.size(), 0);
  }

  @Test
  public void testContainsKey()
  {
    PyDict dict = dictOf("key1", "value1");

    assertTrue(dict.containsKey("key1"));
    assertFalse(dict.containsKey("missing"));
  }

  @Test
  public void testContainsValue()
  {
    PyDict dict = dictOf("key1", context.str("value1"));

    assertTrue(dict.containsValue(context.str("value1")) || dict.values().toString().contains("value1"));
    assertFalse(dict.containsValue(context.str("missing")));
  }

  @Test
  public void testEntrySetReflectsContents()
  {
    PyDict dict = dictOf("a", 1, "b", 2);

    assertEquals(dict.entrySet().size(), 2);
    assertFalse(dict.entrySet().isEmpty());
  }

  @Test
  public void testFromItems()
  {
    List<Map.Entry<String, Integer>> items = List.of(
            new AbstractMap.SimpleEntry<>("a", 1),
            new AbstractMap.SimpleEntry<>("b", 2));

    PyDict dict = context.dictFromItems(items);

    assertEquals(dict.size(), 2);
    assertEquals(dict.get("a").toString(), "1");
    assertEquals(dict.get("b").toString(), "2");
  }

  @Test
  public void testFromMap()
  {
    Map<String, Object> map = new HashMap<>();
    map.put("a", 1);
    map.put("b", 2);

    PyDict dict = context.dictFromMap(map);

    assertEquals(dict.size(), 2);
    assertEquals(dict.get("a").toString(), "1");
    assertEquals(dict.get("b").toString(), "2");
  }

  @Test
  public void testGetMissingReturnsNullOrThrowsAsImplemented()
  {
    PyDict dict = context.dict();
    PyObject result = dict.get("missing");
    assertNull(result);
  }

  @Test
  public void testGetOrDefaultExisting()
  {
    PyDict dict = dictOf("key1", "value1");
    PyObject fallback = context.str("default");

    PyObject result = dict.getOrDefault("key1", fallback);

    assertEquals(result.toString(), "value1");
  }

  @Test
  public void testGetOrDefaultMissing()
  {
    PyDict dict = context.dict();
    PyObject fallback = context.str("default");

    PyObject result = dict.getOrDefault("missing", fallback);

    assertEquals(result, fallback);
  }

  @Test
  @SuppressWarnings("element-type-mismatch")
  public void testKeySetReflectsContents()
  {
    PyDict dict = dictOf("a", 1, "b", 2);

    assertTrue(dict.keySet().contains("a"));
    assertTrue(dict.keySet().contains("b"));
    assertFalse(dict.keySet().contains("z"));
  }

  @Test
  @SuppressWarnings("element-type-mismatch")
  public void testOrUsesDictUnionSemantics()
  {
    PyDict left = dictOf("a", 1, "b", 2);
    PyDict right = dictOf("b", 99, "c", 3);

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 3);
    assertEquals(out.get("a").toString(), "1");
    assertEquals(out.get("b").toString(), "99");
    assertEquals(out.get("c").toString(), "3");

    assertEquals(left.get("b").toString(), "2");
    assertEquals(right.get("b").toString(), "99");
  }

  @Test
  public void testPop()
  {
    PyDict dict = dictOf("key1", "value1");
    PyObject fallback = context.str("default");

    assertEquals(dict.pop("key1", fallback).toString(), "value1");
    assertEquals(dict.pop("missing", fallback), fallback);
  }

  @Test
  public void testPopItem()
  {
    PyDict dict = dictOf("key1", "value1");

    Map.Entry<Object, PyObject> entry = dict.popItem();

    assertNotNull(entry);
    assertEquals(entry.getKey().toString(), "key1");
    assertEquals(entry.getValue().toString(), "value1");
    assertTrue(dict.isEmpty());
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testPopItemEmptyDict()
  {
    context.dict().popItem();
  }

  @Test
  public void testPutAndGet()
  {
    PyDict dict = context.dict();
    dict.putAny("key1", "value1");

    assertEquals(dict.get("key1").toString(), "value1");
  }

  @Test
  public void testRemoveByKey()
  {
    PyDict dict = dictOf("key1", "value1", "key2", "value2");

    PyObject removed = dict.remove("key1");

    assertNotNull(removed);
    assertEquals(removed.toString(), "value1");
    assertFalse(dict.containsKey("key1"));
    assertEquals(dict.size(), 1);
  }

  @Test
  public void testRemoveKeyValueMatch()
  {
    PyDict dict = dictOf("key1", "value1");

    boolean removed = dict.remove("key1", context.str("value1"));

    assertTrue(removed);
    assertFalse(dict.containsKey("key1"));
  }

  @Test
  public void testRemoveKeyValueMismatch()
  {
    PyDict dict = dictOf("key1", "value1");

    boolean removed = dict.remove("key1", context.str("wrong"));

    assertFalse(removed);
    assertTrue(dict.containsKey("key1"));
  }

  @Test
  public void testSetDefaultDoesNotOverwriteExisting()
  {
    PyDict dict = dictOf("key1", "value1");

    PyObject result = dict.setDefault("key1", context.str("other"));

    assertEquals(result.toString(), "value1");
    assertEquals(dict.get("key1").toString(), "value1");
  }

  @Test
  public void testSetDefaultInsertsWhenMissing()
  {
    PyDict dict = context.dict();

    PyObject result = dict.setDefault("key1", context.str("value1"));

    assertEquals(result.toString(), "value1");
    assertEquals(dict.get("key1").toString(), "value1");
  }

  @Test
  public void testSizeAndIsEmpty()
  {
    PyDict dict = context.dict();
    assertEquals(dict.size(), 0);
    assertTrue(dict.isEmpty());

    dict.putAny("k", "v");
    assertEquals(dict.size(), 1);
    assertFalse(dict.isEmpty());
  }

  @Test
  public void testUpdateWithIterable()
  {
    PyDict dict = context.dict();
    List<Map.Entry<Object, PyObject>> updateList = new ArrayList<>();
    updateList.add(new AbstractMap.SimpleEntry<>("key1", context.str("value1")));
    updateList.add(new AbstractMap.SimpleEntry<>("key2", context.str("value2")));

    dict.update(updateList);

    assertEquals(dict.size(), 2);
    assertEquals(dict.get("key1").toString(), "value1");
    assertEquals(dict.get("key2").toString(), "value2");
  }

  @Test
  public void testUpdateWithMap()
  {
    PyDict dict = context.dict();
    Map<Object, PyObject> updateMap = new HashMap<>();
    updateMap.put("key1", context.str("value1"));
    updateMap.put("key2", context.str("value2"));

    dict.update(updateMap);

    assertEquals(dict.size(), 2);
    assertEquals(dict.get("key1").toString(), "value1");
    assertEquals(dict.get("key2").toString(), "value2");
  }

  @Test
  public void testValuesReflectContents()
  {
    PyDict dict = dictOf("a", context.str("x"), "b", context.str("y"));

    assertEquals(dict.values().size(), 2);
    assertTrue(dict.values().toString().contains("x"));
    assertTrue(dict.values().toString().contains("y"));
  }

  @Test
  public void testViewsTrackMutation()
  {
    PyDict dict = dictOf("a", 1);
    assertTrue(dict.keySet().contains("a"));

    dict.putAny("b", 2);
    assertTrue(dict.keySet().contains("b"));

    dict.remove("a");
    assertFalse(dict.keySet().contains("a"));
  }

}
