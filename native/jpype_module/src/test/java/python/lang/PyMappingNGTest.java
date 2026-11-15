// --- file: python/lang/PyMappingNGTest.java ---
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
 * Exercises {@code PyMapping}'s own default methods. JPype's own concrete
 * mapping type (PyDict) shadows all of these with its own overrides, so a
 * custom Python object cast via the structural probe (see
 * plan/GeneratorCastCrash.md) is the only way to reach them. The custom type
 * implements __setitem__/__delitem__ too (registered under
 * collections.abc.Mapping, which the native probe checks - not
 * MutableMapping) so put()/remove()/clear()/putAll() are all reachable.
 */
public class PyMappingNGTest extends PyTestHarness
{

  private PyMapping<?, ?> customMapping()
  {
    context.exec(
            "import _jpype, jpype, collections.abc\n"
            + "class MyMap(collections.abc.Mapping):\n"
            + "    def __init__(self): self._d = {'a': 1, 'b': 2}\n"
            + "    def __getitem__(self, k): return self._d[k]\n"
            + "    def __setitem__(self, k, v): self._d[k] = v\n"
            + "    def __delitem__(self, k): del self._d[k]\n"
            + "    def __iter__(self): return iter(self._d)\n"
            + "    def __len__(self): return len(self._d)\n"
            + "    def clear(self): self._d.clear()\n"
            + "_pi_map = _jpype.pyobject(jpype.JClass('python.lang.PyMapping'), MyMap())\n"
    );
    return (PyMapping<?, ?>) context.eval("_pi_map");
  }

  @Test
  public void testGetPresentAndMissing()
  {
    PyMapping<?, ?> map = customMapping();
    assertEquals(map.get(context.str("a")).toString(), "1");
    assertNull(map.get(context.str("nope")));
  }

  @Test
  public void testContains()
  {
    PyMapping<?, ?> map = customMapping();
    assertTrue(map.contains(context.str("a")));
    assertFalse(map.contains(context.str("nope")));
  }

  @Test
  public void testContainsKeyAndValue()
  {
    PyMapping<?, ?> map = customMapping();
    assertTrue(map.containsKey(context.str("a")));
    assertFalse(map.containsKey(context.str("nope")));
    assertTrue(map.containsValue(context.eval("1")));
    assertFalse(map.containsValue(context.eval("999")));
  }

  @Test
  public void testIsEmpty()
  {
    assertFalse(customMapping().isEmpty());
  }

  @Test
  public void testSize()
  {
    assertEquals(customMapping().size(), 2);
  }

  @Test
  public void testIterator()
  {
    PyMapping<?, ?> map = customMapping();
    Iterator<?> it = map.iterator();
    int count = 0;
    while (it.hasNext())
    {
      it.next();
      count++;
    }
    assertEquals(count, 2);
  }

  @Test
  @SuppressWarnings("unchecked")
  public void testPut()
  {
    PyMapping<PyObject, PyObject> map = (PyMapping<PyObject, PyObject>) customMapping();
    PyObject prev = map.put(context.str("c"), context.eval("3"));
    assertNull(prev);
    assertEquals(map.get(context.str("c")).toString(), "3");
  }

  @Test
  public void testPutAny()
  {
    PyMapping<?, ?> map = customMapping();
    map.putAny("c", 3);
    assertEquals(map.get(context.str("c")).toString(), "3");
  }

  @Test
  @SuppressWarnings("unchecked")
  public void testRemove()
  {
    PyMapping<PyObject, PyObject> map = (PyMapping<PyObject, PyObject>) customMapping();
    PyObject removed = map.remove(context.str("a"));
    assertEquals(removed.toString(), "1");
    assertFalse(map.containsKey(context.str("a")));
  }

  @Test
  public void testClear()
  {
    PyMapping<?, ?> map = customMapping();
    map.clear();
    assertTrue(map.isEmpty());
  }

  @Test
  @SuppressWarnings("unchecked")
  public void testPutAll()
  {
    PyMapping<PyObject, PyObject> map = (PyMapping<PyObject, PyObject>) customMapping();
    java.util.Map<PyObject, PyObject> extra = new java.util.LinkedHashMap<>();
    extra.put(context.str("c"), context.eval("3"));
    extra.put(context.str("d"), context.eval("4"));
    map.putAll(extra);
    assertEquals(map.get(context.str("c")).toString(), "3");
    assertEquals(map.get(context.str("d")).toString(), "4");
  }

  @Test
  public void testKeySet()
  {
    PyMapping<?, ?> map = customMapping();
    java.util.Set<?> keys = map.keySet();
    assertEquals(keys.size(), 2);
  }

  @Test
  public void testValues()
  {
    PyMapping<?, ?> map = customMapping();
    java.util.Collection<?> values = map.values();
    assertEquals(values.size(), 2);
  }

  @Test
  public void testEntrySet()
  {
    PyMapping<?, ?> map = customMapping();
    java.util.Set<?> entries = map.entrySet();
    assertEquals(entries.size(), 2);
  }
}
