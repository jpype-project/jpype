// --- file: python/lang/ProtocolInterfaceCoverageNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises {@code python.lang}'s duck-typed protocol interfaces
 * (PyAbstractSet, PyCollection, PyIterable, PyContainer, PyGenerator) via
 * {@code _jpype.pyobject}, JPype's structural probe that backs an arbitrary
 * Python object with a dynamic proxy - the only way these interfaces'
 * defaults are ever reached, since JPype's own concrete wrapper types
 * (PySet, PyDict, ...) all shadow them. See plan/Coverage.md and
 * plan/GeneratorCastCrash.md.
 */
public class ProtocolInterfaceCoverageNGTest extends PyTestHarness
{

  @Test
  public void testAbstractSetCast()
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MySet:\n"
            + "    def __init__(self, items): self._items = set(items)\n"
            + "    def __contains__(self, x): return x in self._items\n"
            + "    def __iter__(self): return iter(self._items)\n"
            + "    def __len__(self): return len(self._items)\n"
            + "collections.abc.Set.register(MySet)\n"
            + "_pi_abstractset = _jpype.pyobject(jpype.JClass('python.lang.PyAbstractSet'), MySet([1, 2, 3]))\n"
    );
    PyAbstractSet<?> set = (PyAbstractSet<?>) context.eval("_pi_abstractset");
    assertEquals(set.size(), 3);
    assertFalse(set.isEmpty());
  }

  @Test
  public void testCollectionCast()
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MyCollection:\n"
            + "    def __init__(self, items): self._items = list(items)\n"
            + "    def __contains__(self, x): return x in self._items\n"
            + "    def __iter__(self): return iter(self._items)\n"
            + "    def __len__(self): return len(self._items)\n"
            + "collections.abc.Collection.register(MyCollection)\n"
            + "_pi_collection = _jpype.pyobject(jpype.JClass('python.lang.PyCollection'), MyCollection([1, 2]))\n"
    );
    PyCollection<?> coll = (PyCollection<?>) context.eval("_pi_collection");
    assertEquals(coll.size(), 2);
    assertFalse(coll.isEmpty());
  }

  @Test
  public void testIterableCast()
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MyIterable:\n"
            + "    def __init__(self, items): self._items = list(items)\n"
            + "    def __iter__(self): return iter(self._items)\n"
            + "collections.abc.Iterable.register(MyIterable)\n"
            + "_pi_iterable = _jpype.pyobject(jpype.JClass('python.lang.PyIterable'), MyIterable([1, 2, 3]))\n"
    );
    PyIterable<?> iterable = (PyIterable<?>) context.eval("_pi_iterable");
    int count = 0;
    for (Object o : iterable)
      count++;
    assertEquals(count, 3);
  }

  @Test
  public void testContainerCast()
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MyContainer:\n"
            + "    def __init__(self, items): self._items = list(items)\n"
            + "    def __contains__(self, x): return x in self._items\n"
            + "collections.abc.Container.register(MyContainer)\n"
            + "_pi_container = _jpype.pyobject(jpype.JClass('python.lang.PyContainer'), MyContainer([1, 2, 3]))\n"
    );
    PyContainer<?> container = (PyContainer<?>) context.eval("_pi_container");
    assertNotNull(container);

    // PyContainer.contains is the interface's own default method body.
    // Every concrete class in this codebase that implements PyContainer
    // (via PyCollection -> PyAbstractSet/PyMapping/PySequence) overrides
    // contains() itself, so this structural probe - a plain object with
    // only __contains__, cast to the bare PyContainer interface - is the
    // only way to actually reach it.
    assertTrue(container.contains(context.eval("1")));
    assertFalse(container.contains(context.eval("99")));
  }

  // Regression test for the JVM crash documented in
  // plan/GeneratorCastCrash.md: casting a real Python generator to
  // PyGenerator and calling iterator()/next() on the result used to
  // segfault the JVM via infinite mutual recursion (see PyGenerator.java's
  // iterator() default and pyjp_probe.cpp's interrogate()).
  @Test
  public void testGeneratorCastAndIteration()
  {
    context.exec(
            "import _jpype, jpype\n"
            + "def _pi_mygen():\n"
            + "    yield 1\n"
            + "    yield 2\n"
            + "_pi_gen = _jpype.pyobject(jpype.JClass('python.lang.PyGenerator'), _pi_mygen())\n"
    );
    PyGenerator<?> gen = (PyGenerator<?>) context.eval("_pi_gen");
    assertNotNull(gen.iter());

    java.util.Iterator<?> it = gen.iterator();
    assertTrue(it.hasNext());
    assertEquals(it.next().toString(), "1");
    assertTrue(it.hasNext());
    assertEquals(it.next().toString(), "2");
    assertFalse(it.hasNext());
  }
}
