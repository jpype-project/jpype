// --- file: python/lang/PyFrozenSet.java ---
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

import java.util.Collection;
import java.util.Iterator;
import java.util.Set;
import org.jpype.annotation.Bypass;

/**
 * Java front-end interface for the Python `frozenset` type.
 *
 * This interface provides functionality for creating and interacting with
 * Python `frozenset` objects in a Java environment, mimicking Python's
 * immutable set type.<p>
 *
 * The Python `frozenset` getType represents an immutable, hashable collection
 * of unique elements, similar to Java's {@link Set} interface. This interface
 * extends {@link PyObject} and {@link Set}, offering methods to perform set
 * operations such as union, intersection, difference, and more.
 *
 * <p>
 * Note: While this interface mostly adheres to Java's contract for sets, some
 * operations (e.g., {@code add}, {@code update}) may behave differently due to
 * the immutable nature of Python's `frozenset`.
 *
 * <p>
 * <b>Important Note:</b></p>
 * <p>
 * Python collections are asymmetric in their handling of Java objects. A Java
 * object added to a Python collection will appear as a {@code PyJavaObject}.
 * Developers should exercise caution to avoid reference loops when placing Java
 * objects into Python collections, as this may lead to unintended
 * behaviors.</p>
 */
public interface PyFrozenSet extends PyObject, Set<PyObject>
{


  /**
   * Creates a shallow copy of this `frozenset`.
   *
   * @return a new {@link PyFrozenSet} instance containing the same elements as
   * this set.
   */
  PyFrozenSet copy();

  /**
   * Computes the difference between this `frozenset` and one or more other
   * sets.
   *
   * @param set one or more {@link Collection} instances to subtract from this
   * set.
   * @return a new {@link PyFrozenSet} containing elements in this set but not
   * in the specified sets.
   */
  PyFrozenSet difference(Collection<?>... set);

  /**
   * Computes the intersection of this `frozenset` with one or more other sets.
   *
   * @param set one or more {@link Collection} instances to intersect with this
   * set.
   * @return a new {@link PyFrozenSet} containing elements common to all sets.
   */
  PyFrozenSet intersection(Collection<?>... set);

  /**
   * Checks whether this `frozenset` and the specified set are disjoint. Two
   * sets are disjoint if they have no elements in common.
   *
   * @param set the {@link Collection} to compare with.
   * @return {@code true} if the sets are disjoint, {@code false} otherwise.
   */
  boolean isDisjoint(Collection<?> set);

  /**
   * Checks whether this `frozenset` is a subset of the specified set.
   *
   * @param set the {@link Collection} to compare with.
   * @return {@code true} if this set is a subset of the specified set,
   * {@code false} otherwise.
   */
  boolean isSubset(Collection<?> set);

  /**
   * Checks whether this `frozenset` is a superset of the specified set.
   *
   * @param set the {@link Collection} to compare with.
   * @return {@code true} if this set is a superset of the specified set,
   * {@code false} otherwise.
   */
  boolean isSuperset(Collection<?> set);

  /**
   * Removes and returns an arbitrary element from this `frozenset`.
   *
   * @return the removed {@link PyObject}.
   */
    @Bypass
  default PyObject pop()
  {
    throw new UnsupportedOperationException("Frozenset does not support modification");
  }

  /**
   * Computes the symmetric difference between this `frozenset` and one or more
   * other sets. The symmetric difference contains elements that are in either
   * set, but not in both.
   *
   * @param set one or more {@link Set} instances to compare with.
   * @return a new {@link PyFrozenSet} containing the symmetric difference.
   */
  PyFrozenSet symmetricDifference(Collection<?>... set);

  /**
   * Computes the union of this `frozenset` with one or more other sets. The
   * union contains all elements from all sets.
   *
   * @param set one or more {@link Set} instances to combine with this set.
   * @return a new {@link PyFrozenSet} containing the union of all sets.
   */
  PyFrozenSet union(Collection<?>... set);

  /**
   * Returns an iterator over the elements in this `frozenset`.
   *
   * @return an {@link Iterator} for the elements in this set.
   */
    @Bypass
  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator<>(builtin().backend.iterSet(this));
  }

  /**
   * Converts this `frozenset` into an array.
   *
   * @return an array containing all elements in this set.
   */
    @Bypass
  @Override
  default Object[] toArray()
  {
    // Do not route through `new ArrayList<>(this)` - its constructor calls
    // this.toArray() internally, recursing into this same default method
    // and overflowing the stack.
    Object[] result = new Object[size()];
    int i = 0;
    for (Object o : this)
      result[i++] = o;
    return result;
  }

  /**
   * Converts this `frozenset` into an array of the specified getType.
   *
   * @param reference the array into which the elements of this set will be
   * stored.
   * @param <T> the getType of the array elements.
   * @return an array containing all elements in this set.
   */
    @Bypass
  @Override
  @SuppressWarnings("unchecked")
  default <T> T[] toArray(T[] reference)
  {
    int size = size();
    if (reference.length < size)
      reference = (T[]) java.lang.reflect.Array.newInstance(reference.getClass().getComponentType(), size);
    int i = 0;
    Object[] result = reference;
    for (Object o : this)
      result[i++] = o;
    if (reference.length > size)
      reference[size] = null;
    return reference;
  }

}
