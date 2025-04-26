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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Set;
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;

/**
 * Java front-end interface for the Python `frozenset` type. This interface
 * provides functionality for creating and interacting with Python `frozenset`
 * objects in a Java environment, mimicking Python's immutable set type.
 *
 * <p>
 * The Python `frozenset` type represents an immutable, hashable collection of
 * unique elements, similar to Java's {@link Set} interface. This interface
 * extends {@link PyObject} and {@link Set}, offering methods to perform set
 * operations such as union, intersection, difference, and more.
 *
 * <p>
 * Note: While this interface mostly adheres to Java's contract for sets, some
 * operations (e.g., {@code add}, {@code update}) may behave differently due to
 * the immutable nature of Python's `frozenset`.
 */
public interface PyFrozenSet extends PyObject, Set<PyObject>
{

  /**
   * Retrieves the Python type object for `frozenset`. This is equivalent to
   * evaluating `type(frozenset)` in Python.
   *
   * @return the {@link PyType} instance representing the Python `frozenset`
   * type.
   */
  static PyType type()
  {
    return (PyType) BuiltIn.eval("set", null, null);
  }

  /**
   * Creates a new Python `frozenset` object from the specified
   * {@link Iterable}.
   *
   * @param c the {@link Iterable} whose elements will be included in the
   * `frozenset`.
   * @return a new {@link PyFrozenSet} instance representing the Python
   * `frozenset` object.
   */
  static PyFrozenSet of(Iterable c)
  {
    return Interpreter.getBackend().newFrozenSet(c);
  }

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
   * @param set one or more {@link PyFrozenSet} instances to subtract from this
   * set.
   * @return a new {@link PyFrozenSet} containing elements in this set but not
   * in the specified sets.
   */
  PyFrozenSet difference(PyFrozenSet... set);

  /**
   * Removes the specified item from this `frozenset`, if it exists. This
   * operation does not raise an exception if the item is not present.
   *
   * @param item the item to discard.
   */
  void discard(Object item);

  /**
   * Computes the intersection of this `frozenset` with one or more other sets.
   *
   * @param set one or more {@link PyFrozenSet} instances to intersect with this
   * set.
   * @return a new {@link PyFrozenSet} containing elements common to all sets.
   */
  PyFrozenSet intersect(PyFrozenSet... set);

  /**
   * Checks whether this `frozenset` and the specified set are disjoint. Two
   * sets are disjoint if they have no elements in common.
   *
   * @param set the {@link PyFrozenSet} to compare with.
   * @return {@code true} if the sets are disjoint, {@code false} otherwise.
   */
  boolean isDisjoint(PyFrozenSet set);

  /**
   * Checks whether this `frozenset` is a subset of the specified set.
   *
   * @param set the {@link PyFrozenSet} to compare with.
   * @return {@code true} if this set is a subset of the specified set,
   * {@code false} otherwise.
   */
  boolean isSubset(PyFrozenSet set);

  /**
   * Checks whether this `frozenset` is a superset of the specified set.
   *
   * @param set the {@link PyFrozenSet} to compare with.
   * @return {@code true} if this set is a superset of the specified set,
   * {@code false} otherwise.
   */
  boolean isSuperset(PyFrozenSet set);

  /**
   * Removes and returns an arbitrary element from this `frozenset`.
   *
   * @return the removed {@link PyObject}.
   */
  PyObject pop();

  /**
   * Computes the symmetric difference between this `frozenset` and one or more
   * other sets. The symmetric difference contains elements that are in either
   * set, but not in both.
   *
   * @param set one or more {@link PyFrozenSet} instances to compare with.
   * @return a new {@link PyFrozenSet} containing the symmetric difference.
   */
  PyFrozenSet symmetricDifference(PyFrozenSet... set);

  /**
   * Computes the union of this `frozenset` with one or more other sets. The
   * union contains all elements from all sets.
   *
   * @param set one or more {@link PyFrozenSet} instances to combine with this
   * set.
   * @return a new {@link PyFrozenSet} containing the union of all sets.
   */
  PyFrozenSet union(PyFrozenSet... set);

  /**
   * Updates this `frozenset` with elements from another object.
   *
   * @param other the {@link PyObject} whose elements will be added to this set.
   */
  void update(PyObject other);

  /**
   * Returns an iterator over the elements in this `frozenset`.
   *
   * @return an {@link Iterator} for the elements in this set.
   */
  @Override
  default Iterator<PyObject> iterator()
  {
    return Interpreter.getBackend().iter(this).iterator();
  }

  /**
   * Converts this `frozenset` into an array.
   *
   * @return an array containing all elements in this set.
   */
  @Override
  default Object[] toArray()
  {
    return new ArrayList<>(this).toArray();
  }

  /**
   * Converts this `frozenset` into an array of the specified type.
   *
   * @param a the array into which the elements of this set will be stored.
   * @param <T> the type of the array elements.
   * @return an array containing all elements in this set.
   */
  @Override
  default <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList<>(this).toArray(a);
  }

}
