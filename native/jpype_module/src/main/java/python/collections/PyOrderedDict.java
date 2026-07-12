// --- file: python/collections/PyOrderedDict.java ---
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

import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyTuple;

/**
 * Java front-end interface for Python's {@code collections.OrderedDict} —
 * a {@code dict} subclass with a couple of extra order-manipulation
 * methods.
 *
 * <p>
 * Since {@code OrderedDict} is a real {@code dict} subclass, this interface
 * simply extends {@link PyDict} and reuses its entire {@code Map} surface
 * ({@code get}/{@code put}/{@code keySet}/{@code entrySet}/...) — no need
 * to duplicate that plumbing here. Only the two methods {@code dict}
 * itself doesn't have are added: {@link #moveToEnd} and the
 * {@code last}-aware overload of {@link #popItem(boolean)}.
 *
 * <p>
 * Create instances via
 * {@link PyCollections#using(python.lang.PyBuiltIn) PyCollections.using(context)}
 * {@code .orderedDict()}, not directly.
 *
 * <p>
 * Note on {@code popitem}: plain {@link PyDict#popItem()} (no arguments)
 * already matches Python's own default ({@code last=True}, i.e. LIFO), so
 * it needs no override here and continues to work unchanged on an
 * {@code OrderedDict} instance. {@link #popItem(boolean)} is a genuinely
 * new overload — not a shadow of the inherited method — for callers who
 * want to choose FIFO ({@code last=false}) removal explicitly.
 */
public interface PyOrderedDict extends PyDict
{

  /**
   * Moves an existing key to the right end (default) or left end of the
   * dict, without changing its associated value.
   *
   * @param key the key to move. Must already be present.
   * @param last if {@code true} (the default in Python), moves the key to
   * become the most-recently-inserted; if {@code false}, moves it to
   * become the least-recently-inserted.
   * @throws RuntimeException (bridged {@code KeyError}) if {@code key} is
   * not present.
   */
  void moveToEnd(PyObject key, boolean last);

  /**
   * Equivalent to {@code moveToEnd(key, true)} — moves {@code key} to the
   * most-recently-inserted position.
   *
   * @param key the key to move. Must already be present.
   * @throws RuntimeException (bridged {@code KeyError}) if {@code key} is
   * not present.
   */
  default void moveToEnd(PyObject key)
  {
    moveToEnd(key, true);
  }

  /**
   * Removes and returns a {@code (key, value)} pair as a 2-element
   * {@link PyTuple}: the most-recently-inserted one if {@code last} is
   * {@code true} (LIFO, matching plain {@link PyDict#popItem()}), or the
   * least-recently-inserted one (FIFO) if {@code false}.
   *
   * <p>
   * Unlike the inherited zero-argument {@link PyDict#popItem()} (which
   * returns a {@code java.util.Map.Entry} for convenience), this overload
   * returns the raw {@link PyTuple} — callers who want FIFO removal are
   * assumed comfortable with {@code tuple.get(0)}/{@code get(1)}.
   *
   * @param last {@code true} for LIFO removal, {@code false} for FIFO.
   * @return the removed {@code (key, value)} pair.
   * @throws RuntimeException (bridged {@code KeyError}) if the dict is
   * empty.
   */
  PyTuple popItem(boolean last);

}
