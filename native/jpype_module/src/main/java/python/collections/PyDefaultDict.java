// --- file: python/collections/PyDefaultDict.java ---
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

/**
 * Java front-end interface for Python's {@code collections.defaultdict} —
 * a {@code dict} subclass that calls a zero-argument factory to conjure a
 * value for any missing key, instead of raising {@code KeyError}.
 *
 * <p>
 * Since {@code defaultdict} is a real {@code dict} subclass, this interface
 * simply extends {@link PyDict} and reuses its entire {@code Map} surface —
 * including {@code get}, which (matching Python's own semantics) does
 * <em>not</em> invoke the default factory; only Python-level {@code []}
 * item access does. Reaching for that {@code []} auto-vivifying behavior
 * from Java isn't exposed by this interface (bridge item-access always
 * routes through {@code get}), so the sole addition here is read access to
 * the factory itself.
 *
 * <p>
 * Create instances via
 * {@link PyCollections#using(python.lang.PyBuiltIn) PyCollections.using(context)}
 * {@code .defaultDict(factory)}, not directly — the factory callable is a
 * constructor argument in Python, there is no way to attach or change it
 * after the fact.
 */
public interface PyDefaultDict extends PyDict
{

  /**
   * @return the callable used to produce a value for a missing key, or
   * {@code null} if this {@code defaultdict} has no factory (equivalent to
   * a plain {@code dict} for lookup purposes).
   */
  PyObject defaultFactory();

}
