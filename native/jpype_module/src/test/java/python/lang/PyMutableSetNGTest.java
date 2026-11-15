// --- file: python/lang/PyMutableSetNGTest.java ---
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
 * {@link PyMutableSet} has no concrete Java implementer anywhere in this
 * codebase - the only way to reach its {@code contains}/{@code size}
 * default method bodies is to structurally probe a real Python
 * {@code collections.abc.MutableSet} through {@code _jpype.pyobject}.
 *
 * This required a native-side fix (see {@code native/python/pyjp_probe.cpp}
 * and {@code pyjp_module.cpp}): the structural probe's fixed protocol-flag
 * pipeline had no {@code mutable_set} slot, so {@code python.lang.PyMutableSet}
 * could never be matched regardless of registration - only
 * {@code python.lang.PyAbstractSet} was reachable via {@code is_set}. Fixed
 * by adding an {@code abc_mutable_set}/{@code is_mutable_set} check
 * (mirroring the existing {@code is_iterable}/{@code is_iterator}
 * more-specific-wins pattern) and a 16th pipeline slot; the Python side
 * (`jpype/_jbridge.py`'s `_jpype._protocol["mutable_set"] = _PyMutableSet`)
 * was already wired and waiting for it.
 */
public class PyMutableSetNGTest extends PyTestHarness
{

  private PyMutableSet<?> mutableSetOf(String varName, int... items)
  {
    StringBuilder init = new StringBuilder();
    for (int i = 0; i < items.length; i++)
    {
      if (i > 0)
        init.append(", ");
      init.append(items[i]);
    }
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MyMutableSet:\n"
            + "    def __init__(self, items): self._items = set(items)\n"
            + "    def __contains__(self, x): return x in self._items\n"
            + "    def __iter__(self): return iter(self._items)\n"
            + "    def __len__(self): return len(self._items)\n"
            + "    def add(self, x): self._items.add(x)\n"
            + "    def discard(self, x): self._items.discard(x)\n"
            + "collections.abc.MutableSet.register(MyMutableSet)\n"
            + varName + " = _jpype.pyobject(jpype.JClass('python.lang.PyMutableSet'), MyMutableSet([" + init + "]))\n"
    );
    return (PyMutableSet<?>) context.eval(varName);
  }

  @Test
  public void testContainsPresentElement()
  {
    PyMutableSet<?> set = mutableSetOf("_pi_mutset_contains_present", 1, 2, 3);
    assertTrue(set.contains(context.$int(2)));
  }

  @Test
  public void testContainsMissingElement()
  {
    PyMutableSet<?> set = mutableSetOf("_pi_mutset_contains_missing", 1, 2, 3);
    assertFalse(set.contains(context.$int(99)));
  }

  @Test
  public void testSizeReflectsElementCount()
  {
    PyMutableSet<?> set = mutableSetOf("_pi_mutset_size", 1, 2, 3, 4);
    assertEquals(set.size(), 4);
  }

  @Test
  public void testSizeOfEmptySet()
  {
    PyMutableSet<?> set = mutableSetOf("_pi_mutset_empty");
    assertEquals(set.size(), 0);
  }
}
