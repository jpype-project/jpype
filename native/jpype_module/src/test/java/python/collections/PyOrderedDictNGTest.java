// --- file: python/collections/PyOrderedDictNGTest.java ---
package python.collections;

import java.util.Iterator;
import java.util.Map;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyObject;
import python.lang.PyTestHarness;
import python.lang.PyTuple;

public class PyOrderedDictNGTest extends PyTestHarness
{

  private PyOrderedDict withThreeEntries()
  {
    PyOrderedDict od = PyCollections.using(context).orderedDict();
    od.put(context.str("a"), context.$int(1));
    od.put(context.str("b"), context.$int(2));
    od.put(context.str("c"), context.$int(3));
    return od;
  }

  @Test
  public void testCreateEmpty()
  {
    PyOrderedDict od = PyCollections.using(context).orderedDict();

    assertNotNull(od);
    assertTrue(od.isEmpty());
    assertEquals(od.size(), 0);
  }

  @Test
  public void testInheritedDictOperations()
  {
    PyOrderedDict od = withThreeEntries();

    assertEquals(od.size(), 3);
    assertTrue(od.containsKey(context.str("a")));
    assertEquals(od.get(context.str("b")).toString(), "2");

    od.remove(context.str("a"));

    assertFalse(od.containsKey(context.str("a")));
    assertEquals(od.size(), 2);
  }

  @Test
  public void testInsertionOrderPreserved()
  {
    PyOrderedDict od = withThreeEntries();

    StringBuilder sb = new StringBuilder();
    Iterator<PyObject> it = od.keySet().iterator();
    while (it.hasNext())
      sb.append(it.next().toString());

    assertEquals(sb.toString(), "abc");
  }

  @Test
  public void testMoveToEndDefaultLast()
  {
    PyOrderedDict od = withThreeEntries();

    od.moveToEnd(context.str("a"));

    StringBuilder sb = new StringBuilder();
    for (PyObject key : od.keySet())
      sb.append(key.toString());

    assertEquals(sb.toString(), "bca");
  }

  @Test
  public void testMoveToEndFirst()
  {
    PyOrderedDict od = withThreeEntries();

    od.moveToEnd(context.str("c"), false);

    StringBuilder sb = new StringBuilder();
    for (PyObject key : od.keySet())
      sb.append(key.toString());

    assertEquals(sb.toString(), "cab");
  }

  @Test
  public void testPopItemLifo()
  {
    PyOrderedDict od = withThreeEntries();

    Map.Entry<Object, PyObject> entry = od.popItem();

    assertEquals(entry.getKey().toString(), "c");
    assertEquals(od.size(), 2);
  }

  @Test
  public void testPopItemFifo()
  {
    PyOrderedDict od = withThreeEntries();

    PyTuple entry = od.popItem(false);

    assertEquals(entry.get(0).toString(), "a");
    assertEquals(entry.get(1).toString(), "1");
    assertEquals(od.size(), 2);
  }
}
