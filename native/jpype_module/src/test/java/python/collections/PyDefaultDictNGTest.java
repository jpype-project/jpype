// --- file: python/collections/PyDefaultDictNGTest.java ---
package python.collections;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyDefaultDictNGTest extends PyTestHarness
{

  @Test
  public void testCreateWithoutFactory()
  {
    PyDefaultDict dd = PyCollections.using(context).defaultDict();

    assertNotNull(dd);
    assertTrue(dd.isEmpty());
    assertNull(dd.defaultFactory());
  }

  @Test
  public void testDefaultFactoryAccessor()
  {
    PyDefaultDict dd = PyCollections.using(context).defaultDict(context.eval("int"));

    assertNotNull(dd.defaultFactory());
    assertEquals(dd.defaultFactory().toString(), "<class 'int'>");
  }

  @Test
  public void testInheritedDictOperations()
  {
    PyDefaultDict dd = PyCollections.using(context).defaultDict(context.eval("int"));

    dd.put(context.str("a"), context.$int(5));

    assertEquals(dd.size(), 1);
    assertEquals(dd.get(context.str("a")).toString(), "5");
    assertNull(dd.get(context.str("missing")));
  }
}
