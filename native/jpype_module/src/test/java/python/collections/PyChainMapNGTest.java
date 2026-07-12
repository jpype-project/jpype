// --- file: python/collections/PyChainMapNGTest.java ---
package python.collections;

import java.util.Arrays;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyDict;
import python.lang.PyList;
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
}
