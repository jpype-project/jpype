// --- file: python/lang/PyAbstractSetNGTest.java ---
package python.lang;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import static org.testng.Assert.*;
import org.testng.annotations.*;

public class PyAbstractSetNGTest extends PyTestHarness
{

  @Test
  public void testContainsMissingElement()
  {
    PyAbstractSet<PyObject> set = context.set(Arrays.asList("a", "b", "c"));
    assertFalse(set.contains("z"));
  }

  @Test
  public void testContainsPresentElement()
  {
    PyAbstractSet<PyObject> set = context.set(Arrays.asList("a", "b", "c"));
    assertTrue(set.contains("a"));
  }

  @Test
  public void testIsEmptyFalseForNonEmptySet()
  {
    PyAbstractSet<PyObject> set = context.set(Arrays.asList("x"));
    assertFalse(set.isEmpty());
  }

  @Test
  public void testIteratorOnEmptySet()
  {
    PyAbstractSet<PyObject> set = context.set(Arrays.asList());
    assertFalse(set.iterator().hasNext());
  }

  public void testIteratorTraversesElements()
  {
    PyAbstractSet<PyObject> set = context.set(Arrays.asList("a", "b", "c"));

    Set<String> actual = new HashSet<>();
    for (PyObject obj : set)
      actual.add(obj.toString());

    assertEquals(actual.size(), 3);
    assertTrue(actual.contains("a"));
    assertTrue(actual.contains("b"));
    assertTrue(actual.contains("c"));
  }

  @Test
  public void testOfCreatesSet()
  {
    PySet set = context.set(Arrays.asList("a", "b", "c"));
    assertNotNull(set);
    assertEquals(set.size(), 3);
  }

  @Test
  public void testOfEmptyIterable()
  {
    PySet set = context.set(Arrays.asList());
    assertNotNull(set);
    assertTrue(set.isEmpty());
    assertEquals(set.size(), 0);
  }

  @Test
  public void testSizeWithDuplicates()
  {
    PyAbstractSet<PyObject> set = context.set(Arrays.asList("a", "a", "b"));
    assertEquals(set.size(), 2);
  }
}
