// --- file: python/collections/PyCounter.java ---
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
package python.collections;

import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import python.lang.PyDict;
import python.lang.PyIter;
import python.lang.PyIterator;
import python.lang.PyList;
import python.lang.PyObject;
import python.lang.PyTuple;

/**
 * Java front-end interface for Python's {@code collections.Counter} — a
 * {@code dict[T, int]} subclass for tallying hashable objects, with a
 * handful of counting-specific extras layered on top of the plain
 * {@code dict} protocol.
 *
 * <p>
 * Since {@code Counter} is a real {@code dict} subclass, this interface
 * extends {@link PyDict} and reuses its entire {@code Map} surface. Note
 * that {@link PyDict#get} keeps plain {@code dict.get()} semantics —
 * {@code null} for a missing key — because Python's {@code dict.get()}
 * does not go through {@code __missing__}. {@code Counter}'s well-known
 * "missing keys count as zero" behavior only fires via {@code __getitem__}
 * (Python's {@code counter[key]}), which is what {@link #getCount} calls.
 *
 * <p>
 * Create instances via
 * {@link PyCollections#using(python.lang.PyBuiltIn) PyCollections.using(context)}
 * {@code .counter()} / {@code .counter(iterable)} / {@code .counter(map)},
 * not directly.
 *
 * <p>
 * <b>{@code update} has different semantics than plain {@code dict}:</b>
 * Python's {@code Counter.update} is <em>additive</em> — it adds counts
 * rather than overwriting them, unlike {@code dict.update}'s
 * overwrite-in-place behavior. Since this is exactly what Python's own
 * {@code Counter} class already does when its inherited {@code update}
 * method is called (the override happens once, at the Python level — Java
 * just calls straight through {@link PyDict#update}), no new method is
 * needed here; this Javadoc note exists purely so callers aren't surprised
 * that {@link PyDict#update(Map)}/{@link PyDict#update(Iterable)} behave
 * additively on a {@code PyCounter}, unlike on a plain {@link PyDict}.
 */
public interface PyCounter extends PyDict
{

  /**
   * @return every distinct element with its count, most-common first (ties
   * broken by first-encountered order, matching Python).
   */
  default List<Map.Entry<PyObject, Integer>> mostCommon()
  {
    return toEntries(mostCommonAllRaw());
  }

  /**
   * @param n how many of the most-common elements to return.
   * @return the {@code n} most-common elements with their counts,
   * most-common first.
   */
  default List<Map.Entry<PyObject, Integer>> mostCommon(int n)
  {
    return toEntries(mostCommonRaw(n));
  }

  /**
   * Raw SPI-backed implementation of {@link #mostCommon()} — a Python list
   * of {@code (element, count)} 2-tuples, most-common first.
   */
  PyList mostCommonAllRaw();

  /**
   * Raw SPI-backed implementation of {@link #mostCommon(int)} — a Python
   * list of {@code (element, count)} 2-tuples, most-common first.
   *
   * @param n how many of the most-common elements to return.
   */
  PyList mostCommonRaw(int n);

  /**
   * @return an iterable yielding each element as many times as its count
   * (elements with a count &lt;= 0 are skipped, matching Python). Iteration
   * order matches insertion order of the counted elements.
   */
  default Iterable<PyObject> elements()
  {
    return () -> new PyIterator<>(elementsRaw());
  }

  /**
   * Raw SPI-backed implementation of {@link #elements()}.
   */
  PyIter<PyObject> elementsRaw();

  /**
   * Looks up an element's count via Python's {@code counter[key]}
   * ({@code __getitem__}), which returns {@code 0} for a key that isn't
   * present rather than raising or returning {@code null}. Prefer this
   * over the inherited {@link PyDict#get} when you want Counter's
   * zero-default behavior.
   *
   * @param key the element to look up.
   * @return the element's count, or {@code 0} if not present.
   */
  int getCount(PyObject key);

  /**
   * Subtracts counts, element-wise, using the counts in {@code m}. Unlike
   * {@link #update(Map)}, counts (including result counts) may go
   * negative.
   *
   * @param m the map of elements to counts to subtract.
   */
  void subtract(Map<? extends Object, ? extends Integer> m);

  /**
   * Subtracts one from the count of each element in {@code iterable} (an
   * element may appear more than once, subtracting once per occurrence).
   *
   * @param iterable the elements whose counts should be decremented.
   */
  void subtract(Iterable<? extends Object> iterable);

  /**
   * @return the sum of every element's count. Available since Python 3.10.
   */
  int total();

  private static List<Map.Entry<PyObject, Integer>> toEntries(PyList raw)
  {
    List<Map.Entry<PyObject, Integer>> out = new ArrayList<>(raw.size());
    for (PyObject o : raw)
    {
      PyTuple t = (PyTuple) o;
      PyObject key = t.get(0);
      int count = (int) t.builtin().asLong(t.get(1));
      out.add(new AbstractMap.SimpleEntry<>(key, count));
    }
    return out;
  }

}
