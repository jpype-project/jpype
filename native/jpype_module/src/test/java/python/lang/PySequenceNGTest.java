// --- file: python/lang/PySequenceNGTest.java ---
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

import java.util.Iterator;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises {@code PySequence}'s own default methods (contains, get(int),
 * get(PySubscript), get(PySubscript...), isEmpty, iterator, size), which
 * JPype's own concrete sequence types (PyList, PyTuple, ...) all shadow with
 * their own overrides - so a custom Python object cast via the structural
 * probe (see plan/GeneratorCastCrash.md) is the only way to reach them.
 */
public class PySequenceNGTest extends PyTestHarness
{

  private PySequence<?> customSequence(String pyItems)
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MySeq:\n"
            + "    def __init__(self, items): self._items = list(items)\n"
            + "    def __getitem__(self, i):\n"
            + "        if isinstance(i, tuple) and len(i) == 1: i = i[0]\n"
            + "        return self._items[i]\n"
            + "    def __len__(self): return len(self._items)\n"
            + "collections.abc.Sequence.register(MySeq)\n"
            + "_pi_seq = _jpype.pyobject(jpype.JClass('python.lang.PySequence'), MySeq(" + pyItems + "))\n"
    );
    return (PySequence<?>) context.eval("_pi_seq");
  }

  @Test
  public void testGetInt()
  {
    PySequence<?> seq = customSequence("[10, 20, 30]");
    assertEquals(seq.get(0).toString(), "10");
    assertEquals(seq.get(2).toString(), "30");
  }

  @Test
  public void testContains()
  {
    PySequence<?> seq = customSequence("[1, 2, 3]");
    assertTrue(seq.contains(context.eval("2")));
    assertFalse(seq.contains(context.eval("99")));
  }

  @Test
  public void testIsEmptyFalse()
  {
    PySequence<?> seq = customSequence("[1]");
    assertFalse(seq.isEmpty());
  }

  @Test
  public void testIsEmptyTrue()
  {
    PySequence<?> seq = customSequence("[]");
    assertTrue(seq.isEmpty());
  }

  @Test
  public void testIterator()
  {
    PySequence<?> seq = customSequence("[1, 2, 3]");
    Iterator<?> it = seq.iterator();
    int count = 0;
    while (it.hasNext())
    {
      it.next();
      count++;
    }
    assertEquals(count, 3);
  }

  @Test
  public void testSize()
  {
    PySequence<?> seq = customSequence("[1, 2, 3, 4]");
    assertEquals(seq.size(), 4);
  }

  @Test
  public void testGetWithSlice()
  {
    PySequence<?> seq = customSequence("[1, 2, 3, 4, 5]");
    PyObject result = seq.get(context.slice(1, 3));
    assertEquals(result.toString(), "[2, 3]");
  }

  @Test
  public void testGetWithSliceVarargs()
  {
    PySequence<?> seq = customSequence("[1, 2, 3, 4, 5]");
    PySubscript[] indices = new PySubscript[]
    {
      context.slice(1, 3)
    };
    PyObject result = seq.get(indices);
    assertEquals(result.toString(), "[2, 3]");
  }
}
