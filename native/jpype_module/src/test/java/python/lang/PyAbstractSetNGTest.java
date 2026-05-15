package python.lang;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

public class PyAbstractSetNGTest
{
  @BeforeClass
  public static void setUpClass() throws Exception
  {
    if (!Interpreter.getInstance().isStarted())
      Interpreter.getInstance().start(new String[0]);
  }

  @Test
  public void testOfCreatesSet()
  {
    PySet set = PyAbstractSet.of(Arrays.asList("a", "b", "c"));
    assertNotNull(set);
    assertEquals(set.size(), 3);
  }

  @Test
  public void testOfEmptyIterable()
  {
    PySet set = PyAbstractSet.of(Arrays.asList());
    assertNotNull(set);
    assertTrue(set.isEmpty());
    assertEquals(set.size(), 0);
  }

  @Test
  public void testContainsPresentElement()
  {
    PyAbstractSet<PyObject> set = PyAbstractSet.of(Arrays.asList("a", "b", "c"));
    assertTrue(set.contains("a"));
  }

  @Test
  public void testContainsMissingElement()
  {
    PyAbstractSet<PyObject> set = PyAbstractSet.of(Arrays.asList("a", "b", "c"));
    assertFalse(set.contains("z"));
  }

  @Test
  public void testSizeWithDuplicates()
  {
    PyAbstractSet<PyObject> set = PyAbstractSet.of(Arrays.asList("a", "a", "b"));
    assertEquals(set.size(), 2);
  }

  @Test
  public void testIsEmptyFalseForNonEmptySet()
  {
    PyAbstractSet<PyObject> set = PyAbstractSet.of(Arrays.asList("x"));
    assertFalse(set.isEmpty());
  }

  @Test
  public void testIteratorTraversesElements()
  {
    PyAbstractSet<PyObject> set = PyAbstractSet.of(Arrays.asList("a", "b", "c"));

    Set<String> actual = new HashSet<>();
    for (PyObject obj : set)
      actual.add(obj.toString());

    assertEquals(actual.size(), 3);
    assertTrue(actual.contains("a"));
    assertTrue(actual.contains("b"));
    assertTrue(actual.contains("c"));
  }

  @Test
  public void testIteratorOnEmptySet()
  {
    PyAbstractSet<PyObject> set = PyAbstractSet.of(Arrays.asList());
    assertFalse(set.iterator().hasNext());
  }
}