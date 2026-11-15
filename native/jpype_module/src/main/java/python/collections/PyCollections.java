// --- file: python/collections/PyCollections.java ---
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

import java.util.Map;
import python.lang.PyBuiltIn;
import python.lang.PyObject;

/**
 * Entry point for constructing {@code python.collections} objects.
 *
 * Named {@code PyCollections} (not {@code Collections}) to avoid colliding
 * with {@link java.util.Collections}.
 *
 * Call {@link #using(PyBuiltIn)} with the interpreter's {@code PyBuiltIn}
 * (e.g. {@code obj.builtin()} for a live {@code PyObject}, or the
 * {@code PyBuiltIn}/{@code Script} already in hand) to get a
 * {@code PyCollections} instance, then use its factory methods to create
 * {@code python.collections} objects:
 *
 * <pre>{@code
 * PyCollections collections = PyCollections.using(context);
 * PyDeque d = collections.deque();
 * PyCounter c = collections.counter(context.listFromObjects(a, a, b));
 * PyOrderedDict od = collections.orderedDict();
 * PyDefaultDict dd = collections.defaultDict(context.eval("int"));
 * }</pre>
 */
public interface PyCollections
{

  static PyCollections using(PyBuiltIn context)
  {
    return context.getBackend(PyCollections.class);
  }

  /**
   * Creates a new, empty {@code collections.deque}.
   *
   * @return a new {@link PyDeque} instance.
   */
  PyDeque deque();

  /**
   * Creates a new {@code collections.deque} pre-populated with the
   * elements of {@code iterable}, in order.
   *
   * @param iterable the initial contents of the deque.
   * @return a new {@link PyDeque} instance.
   */
  PyDeque deque(Iterable<? extends PyObject> iterable);

  /**
   * Creates a new, length-bounded {@code collections.deque} pre-populated
   * with the elements of {@code iterable}. Once {@code maxlen} elements are
   * present, further appends discard from the opposite end.
   *
   * @param iterable the initial contents of the deque.
   * @param maxlen the maximum number of elements the deque may hold.
   * @return a new {@link PyDeque} instance.
   */
  PyDeque deque(Iterable<? extends PyObject> iterable, int maxlen);

  /**
   * Creates a new, empty {@code collections.Counter}.
   *
   * @return a new {@link PyCounter} instance.
   */
  PyCounter counter();

  /**
   * Creates a new {@code collections.Counter}, tallying one occurrence for
   * each element yielded by {@code iterable}.
   *
   * @param iterable the elements to tally.
   * @return a new {@link PyCounter} instance.
   */
  PyCounter counter(Iterable<? extends PyObject> iterable);

  /**
   * Creates a new {@code collections.Counter} pre-populated with the given
   * element-to-count mapping.
   *
   * @param counts the initial element-to-count mapping.
   * @return a new {@link PyCounter} instance.
   */
  PyCounter counter(Map<? extends Object, ? extends Integer> counts);

  /**
   * Creates a new, empty {@code collections.defaultdict} with no default
   * factory (behaves like a plain {@code dict} for lookups, but remains a
   * {@code defaultdict} instance).
   *
   * @return a new {@link PyDefaultDict} instance.
   */
  PyDefaultDict defaultDict();

  /**
   * Creates a new, empty {@code collections.defaultdict} whose missing-key
   * value is produced by calling {@code factory} with no arguments.
   *
   * @param factory a zero-argument callable, e.g. the builtin {@code int}
   * or {@code list} types, or a user-defined Python function.
   * @return a new {@link PyDefaultDict} instance.
   */
  PyDefaultDict defaultDict(PyObject factory);

  /**
   * Creates a new, empty {@code collections.OrderedDict}.
   *
   * @return a new {@link PyOrderedDict} instance.
   */
  PyOrderedDict orderedDict();

  /**
   * Creates a new {@code collections.ChainMap} over the given mappings,
   * searched in the given order. Writes and deletes always land on the
   * first element of {@code maps}.
   *
   * @param maps the mappings to chain, first-searched first. Must be
   * non-empty.
   * @return a new {@link PyChainMap} instance.
   */
  PyChainMap chainMap(Iterable<? extends PyObject> maps);

}
