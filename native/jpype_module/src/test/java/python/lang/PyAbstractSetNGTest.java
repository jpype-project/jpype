// --- file: python/lang/PyAbstractSetNGTest.java ---
package python.lang;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
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

  // PyAbstractSet's own contains()/iterator() default method bodies are
  // never reached through context.set(...) above: that always returns a
  // concrete PySet, and PySet overrides both contains() and iterator()
  // itself. Reaching the interface's own defaults requires a structural
  // probe - a plain Python object implementing collections.abc.Set, cast
  // to the bare PyAbstractSet interface via _jpype.pyobject - exactly as
  // ProtocolInterfaceCoverageNGTest does for the sibling protocol
  // interfaces.
  private PyAbstractSet<?> probedSet(String varName, String pyItems)
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MyAbstractSet:\n"
            + "    def __init__(self, items): self._items = set(items)\n"
            + "    def __contains__(self, x): return x in self._items\n"
            + "    def __iter__(self): return iter(self._items)\n"
            + "    def __len__(self): return len(self._items)\n"
            + "collections.abc.Set.register(MyAbstractSet)\n"
            + varName + " = _jpype.pyobject(jpype.JClass('python.lang.PyAbstractSet'), MyAbstractSet(" + pyItems + "))\n"
    );
    return (PyAbstractSet<?>) context.eval(varName);
  }

  @Test
  public void testDefaultContainsViaStructuralProbe()
  {
    PyAbstractSet<?> set = probedSet("_pi_absset_contains", "[1, 2, 3]");

    assertTrue(set.contains(context.eval("2")));
    assertFalse(set.contains(context.eval("99")));
  }

  @Test
  public void testDefaultIteratorViaStructuralProbe()
  {
    PyAbstractSet<?> set = probedSet("_pi_absset_iter", "[1, 2, 3]");

    Set<String> seen = new HashSet<>();
    Iterator<?> it = set.iterator();
    while (it.hasNext())
      seen.add(it.next().toString());

    assertEquals(seen.size(), 3);
    assertTrue(seen.contains("1"));
    assertTrue(seen.contains("2"));
    assertTrue(seen.contains("3"));
  }
}
