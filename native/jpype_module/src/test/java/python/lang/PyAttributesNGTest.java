// --- file: python/lang/PyAttributesNGTest.java ---
package python.lang;

import java.util.HashMap;
import java.util.Map;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyAttributesNGTest extends PyTestHarness
{


  private PyObject newObjectWithAttributes()
  {
    context.exec(
            "class AttrTest:\n"
            + "    pass\n"
            + "obj = AttrTest()\n"
            + "obj.name = 'alice'\n"
            + "obj.value = 42\n");
    return (PyObject) context.eval("obj");
  }

  @Test
  public void testClear()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    attrs.clear();

    assertTrue(attrs.isEmpty());
  }

  @Test
  public void testContainsAttribute()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    assertTrue(attrs.contains("name"));
    assertFalse(attrs.contains("missing"));
  }

  @Test
  public void testContainsKey()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    assertTrue(attrs.containsKey(context.str("name")));
    assertFalse(attrs.containsKey(context.str("missing")));
  }

  // Note: containsValue's implementation (PyAttributes.java) calls
  // `builtin.vars(this)` rather than `builtin.vars(obj)` - i.e. it looks up
  // `vars()` of the PyAttributes wrapper itself, not the wrapped Python
  // object. If that turns out to be a real bug rather than something the
  // bridge quietly tolerates, this test is expected to be the one that
  // surfaces it.
  @Test
  public void testContainsValue()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    assertTrue(attrs.containsValue(context.str("alice")));
    assertFalse(attrs.containsValue(context.str("nobody")));
  }

  @Test
  public void testSizeReflectsAttributeCount()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    assertEquals(attrs.size(), 2);

    attrs.put(context.str("extra"), context.$int(1));
    assertEquals(attrs.size(), 3);
  }

  @Test
  public void testDirContainsKnownAttributes()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    PyList dir = attrs.dir();

    assertNotNull(dir);
    assertTrue(dir.contains(context.str("name")) || dir.toString().contains("name"));
  }

  @Test
  public void testEntrySetNotEmpty()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    assertFalse(attrs.entrySet().isEmpty());
  }

  @Test
  public void testGetExistingAttribute()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    PyObject result = attrs.get("name");

    assertNotNull(result);
    assertEquals(result.toString(), "alice");
  }

  @Test
  public void testGetOrDefaultExistingAttribute()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    PyObject result = attrs.getOrDefault("name", context.str("default"));

    assertEquals(result.toString(), "alice");
  }

  @Test
  public void testGetOrDefaultMissingAttribute()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);
    PyObject defaultValue = context.str("default");

    PyObject result = attrs.getOrDefault("missing", defaultValue);

    assertEquals(result, defaultValue);
  }

  @Test
  public void testKeySetReflectsAttributes()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    assertTrue(attrs.keySet().toString().contains("name"));
    assertTrue(attrs.keySet().toString().contains("value"));
  }

  @Test
  public void testPutAll()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    Map<PyObject, PyObject> map = new HashMap<>();
    map.put(context.str("x"), context.$int(1));
    map.put(context.str("y"), context.$int(2));

    attrs.putAll(map);

    assertEquals(attrs.get("x").toString(), "1");
    assertEquals(attrs.get("y").toString(), "2");
  }

  @Test
  public void testPutAttribute()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    attrs.put(context.str("city"), context.str("Paris"));

    assertTrue(attrs.contains("city"));
    assertEquals(attrs.get("city").toString(), "Paris");
  }

  @Test
  public void testRemoveAttribute()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    PyObject removed = attrs.remove("name");

    assertNotNull(removed);
    assertEquals(removed.toString(), "alice");
    assertFalse(attrs.contains("name"));
  }

  @Test
  public void testSizeAndIsEmpty()
  {
    context.exec(
            "class EmptyAttrTest:\n"
            + "    pass\n"
            + "emptyObj = EmptyAttrTest()\n");
    PyObject obj = (PyObject) context.eval("emptyObj");
    PyAttributes attrs = new PyAttributes(obj);

    assertTrue(attrs.isEmpty() || attrs.size() == 0);
  }

  @Test
  public void testValuesReflectAttributes()
  {
    PyObject obj = newObjectWithAttributes();
    PyAttributes attrs = new PyAttributes(obj);

    String values = attrs.values().toString();
    assertTrue(values.contains("alice"));
    assertTrue(values.contains("42"));
  }

}
