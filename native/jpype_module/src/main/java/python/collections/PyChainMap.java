// --- file: python/collections/PyChainMap.java ---
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

import python.lang.PyCombinable;
import python.lang.PyList;
import python.lang.PyMapping;
import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code collections.ChainMap} — a
 * view over several mappings searched in order, with writes/deletes only
 * ever touching the first one.
 *
 * <p>
 * Unlike {@link PyOrderedDict}, {@link PyDefaultDict}, and
 * {@link PyCounter}, {@code ChainMap} is <b>not</b> a real Python
 * {@code dict} subclass (it implements {@code collections.abc.MutableMapping}),
 * so this interface extends {@link PyMapping} rather than
 * {@link python.lang.PyDict} — the two look similar on the Java side, but
 * only {@code PyMapping} is an accurate claim about {@code ChainMap}'s
 * real Python type. It reuses {@code PyMapping}'s entire {@code Map}
 * surface for lookups/writes (which always land on {@link #maps()}'s first
 * element), and adds {@code ChainMap}'s own extras: {@link #maps()},
 * {@link #newChild()}/{@link #newChild(PyObject)}, and {@link #parents()}.
 *
 * <p>
 * {@code ChainMap} also supports the {@code |} merge operator (Python
 * 3.9+), hence {@link PyCombinable}.
 *
 * <p>
 * Create instances via
 * {@link PyCollections#using(python.lang.PyBuiltIn) PyCollections.using(context)}
 * {@code .chainMap(...)}, not directly.
 */
public interface PyChainMap extends PyMapping<PyObject, PyObject>, PyCombinable
{

  /**
   * @return the underlying list of mappings, in search order — the first
   * element is where writes and deletes land. This is a live view: for a
   * real {@code ChainMap} in Python, mutating the returned list (e.g.
   * {@code maps().add(...)}) changes which mappings this {@code ChainMap}
   * searches.
   */
  PyList maps();

  /**
   * @return a new {@code ChainMap} with a fresh, empty mapping inserted at
   * the front (so writes land there instead of {@code this}'s current
   * first mapping), followed by this map's own mappings.
   */
  PyChainMap newChild();

  /**
   * @param m the mapping to insert at the front, instead of a fresh empty
   * one.
   * @return a new {@code ChainMap} with {@code m} inserted at the front,
   * followed by this map's own mappings.
   */
  PyChainMap newChild(PyObject m);

  /**
   * @return a new {@code ChainMap} containing all of this map's mappings
   * except the first (i.e. skips the one writes/deletes would otherwise
   * land on). Useful for looking a key up starting one level further out.
   */
  PyChainMap parents();

}
