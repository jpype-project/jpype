// --- file: python/collections/PyCounterNGTest.java ---
package python.collections;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyObject;
import python.lang.PyTestHarness;

public class PyCounterNGTest extends PyTestHarness
{

  @Test
  public void testCreateEmpty()
  {
    PyCounter c = PyCollections.using(context).counter();

    assertNotNull(c);
    assertTrue(c.isEmpty());
    assertEquals(c.total(), 0);
  }

  @Test
  public void testCreateFromIterable()
  {
    PyCounter c = PyCollections.using(context).counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("b")));

    assertEquals(c.get(context.str("a")).toString(), "2");
    assertEquals(c.get(context.str("b")).toString(), "1");
    assertEquals(c.total(), 3);
  }

  @Test
  public void testCreateFromMap()
  {
    Map<PyObject, Integer> counts = new HashMap<>();
    counts.put(context.str("x"), 5);
    counts.put(context.str("y"), 2);

    PyCounter c = PyCollections.using(context).counter(counts);

    assertEquals(c.get(context.str("x")).toString(), "5");
    assertEquals(c.get(context.str("y")).toString(), "2");
    assertEquals(c.total(), 7);
  }

  @Test
  public void testMissingKeyDefaultsToZero()
  {
    PyCounter c = PyCollections.using(context).counter();

    assertNull(c.get(context.str("missing")));
    assertEquals(c.getCount(context.str("missing")), 0);
  }

  @Test
  public void testMostCommonAll()
  {
    PyCounter c = PyCollections.using(context).counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("a"),
                    context.str("b"), context.str("b"), context.str("c")));

    List<Map.Entry<PyObject, Integer>> mc = c.mostCommon();

    assertEquals(mc.size(), 3);
    assertEquals(mc.get(0).getKey().toString(), "a");
    assertEquals((int) mc.get(0).getValue(), 3);
  }

  @Test
  public void testMostCommonN()
  {
    PyCounter c = PyCollections.using(context).counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("a"),
                    context.str("b"), context.str("b"), context.str("c")));

    List<Map.Entry<PyObject, Integer>> mc = c.mostCommon(2);

    assertEquals(mc.size(), 2);
    assertEquals(mc.get(0).getKey().toString(), "a");
    assertEquals(mc.get(1).getKey().toString(), "b");
  }

  @Test
  public void testElements()
  {
    PyCounter c = PyCollections.using(context).counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("b")));

    StringBuilder sb = new StringBuilder();
    for (PyObject o : c.elements())
      sb.append(o.toString());

    // order not guaranteed across distinct keys in general, but count per key is.
    long aCount = sb.toString().chars().filter(ch -> ch == 'a').count();
    long bCount = sb.toString().chars().filter(ch -> ch == 'b').count();
    assertEquals(aCount, 2);
    assertEquals(bCount, 1);
  }

  @Test
  public void testSubtractWithIterable()
  {
    PyCounter c = PyCollections.using(context).counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("b")));

    c.subtract(context.listFromObjects(context.str("a")));

    assertEquals(c.get(context.str("a")).toString(), "1");
    assertEquals(c.get(context.str("b")).toString(), "1");
  }

  @Test
  public void testSubtractWithMap()
  {
    PyCounter c = PyCollections.using(context).counter(
            context.listFromObjects(context.str("a"), context.str("a"), context.str("b")));

    Map<PyObject, Integer> other = new HashMap<>();
    other.put(context.str("a"), 5);

    c.subtract(other);

    assertEquals(c.get(context.str("a")).toString(), "-3");
  }

  @Test
  public void testInheritedDictOperations()
  {
    PyCounter c = PyCollections.using(context).counter();

    c.put(context.str("z"), context.$int(9));

    assertEquals(c.size(), 1);
    assertEquals(c.get(context.str("z")).toString(), "9");
  }
}
