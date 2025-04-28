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
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.Spliterator;
import java.util.Spliterators;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;
import org.jpype.bridge.Interpreter;

/**
 * Represents a Python set in the Java environment.
 *
 * This interface provides a Java front end for Python's concrete set type,
 * bridging the gap between Python's set operations and Java's {@link Set}
 * interface. It mostly adheres to the Java contract for sets while
 * incorporating Python-specific behaviors and operations.
 *
 * <p>
 * Key features include:</p>
 * <ul>
 * <li>Support for Python-specific set operations (e.g., union, intersection,
 * difference).</li>
 * <li>Integration with Java's {@link Collection} framework.</li>
 * <li>Ability to create and manipulate sets using Python semantics.</li>
 * </ul>
 *
 * <p>
 * All methods in this interface are designed to work seamlessly with Python
 * objects ({@link PyObject}) and provide a consistent API for interacting with
 * Python sets.</p>
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
public interface PySet extends PyAbstractSet<PyObject>
{

  /**
   * Creates a new Python set from the elements of the given {@link Iterable}.
   *
   * @param c an iterable providing elements for the set
   * @param <T> the getType of elements in the iterable
   * @return a new {@code PySet} containing the elements from the iterable
   */
  public static <T> PySet of(Iterable<T> c)
  {
    return Interpreter.getBackend().newSetFromIterable(c);
  }

  /**
   * Adds a single element to the set.
   *
   * @param e the element to add
   * @return {@code true} if the set was modified as a result of this operation,
   * {@code false} otherwise
   */
  @Override
  boolean add(PyObject e);

  void addAny(Object o);

  /**
   * Adds all elements from the specified collection to the set.
   *
   * @param collection the collection containing elements to be added
   * @return {@code true} if the set was modified as a result of this operation,
   * {@code false} otherwise
   */
  @Override
  default boolean addAll(Collection<? extends PyObject> collection)
  {
    int l1 = this.size();
    this.update(of(collection));
    int l2 = this.size();
    return l1 != l2;
  }

  /**
   * Removes all elements from the set.
   */
  @Override
  void clear();

  /**
   * Checks if the set contains the specified element.
   *
   * @param o the element to check for
   * @return {@code true} if the set contains the element, {@code false}
   * otherwise
   */
  @Override
  boolean contains(Object o);

  /**
   * Checks if the set contains all elements from the specified collection.
   *
   * @param collection the collection of elements to check
   * @return {@code true} if the set contains all elements, {@code false}
   * otherwise
   */
  @Override
  default boolean containsAll(Collection<?> collection)
  {
    PySet set2 = of(collection);
    return set2.isSubset(this);
  }

  /**
   * Creates a shallow copy of the set.
   *
   * @return a new {@code PySet} containing the same elements as this set
   */
  PySet copy();

  /**
   * Returns a new set containing the difference between this set and the
   * specified sets.
   *
   * @param set the sets to subtract from this set
   * @return a new {@code PySet} containing the difference
   */
  PySet difference(Collection<?>... set);

  /**
   * Updates this set to contain the difference between itself and the specified
   * sets.
   *
   * @param set the sets to subtract from this set
   */
  void differenceUpdate(Collection<?>... set);

  /**
   * Removes the specified element from the set, if it exists.
   *
   * @param item the element to remove
   */
  void discard(Object item);

  /**
   * Checks if this set is equal to the specified object.
   *
   * @param obj the object to compare with
   * @return {@code true} if the sets are equal, {@code false} otherwise
   */
  @Override
  boolean equals(Object obj);

  /**
   * Returns the hash code for this set.
   *
   * @return the hash code of the set
   */
  @Override
  int hashCode();

  /**
   * Returns a new set containing the intersection of this set and the specified
   * sets.
   *
   * @param set the sets to intersect with this set
   * @return a new {@code PySet} containing the intersection
   */
  PySet intersect(Collection<?>... set);

  /**
   * Updates this set to contain the intersection of itself and the specified
   * sets.
   *
   * @param set the sets to intersect with this set
   */
  void intersectionUpdate(Collection<?>... set);

  /**
   * Checks if this set is disjoint with the specified set.
   *
   * @param set the set to compare with
   * @return {@code true} if the sets are disjoint, {@code false} otherwise
   */
  boolean isDisjoint(Collection<?> set);

  /**
   * Checks if the set is empty.
   *
   * @return {@code true} if the set contains no elements, {@code false}
   * otherwise
   */
  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Checks if this set is a subset of the specified set.
   *
   * @param set the set to compare with
   * @return {@code true} if this set is a subset, {@code false} otherwise
   */
  boolean isSubset(Collection<?> set);

  /**
   * Checks if this set is a superset of the specified set.
   *
   * @param set the set to compare with
   * @return {@code true} if this set is a superset, {@code false} otherwise
   */
  boolean isSuperset(Collection<?> set);

  /**
   * Returns an iterator over the elements in this set.
   *
   * @return an iterator for the set
   */
  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator<>(Interpreter.getBackend().iterSet(this));
  }

  /**
   * Returns a parallel {@link Stream} of the elements in this set.
   *
   * @return a parallel stream of the set's elements
   */
  @Override
  default Stream<PyObject> parallelStream()
  {
    return StreamSupport.stream(this.spliterator(), true);
  }

  /**
   * Removes and returns an arbitrary element from the set.
   *
   * @return the removed element
   */
  PyObject pop();

  /**
   * Removes the specified element from the set.
   *
   * @param o the element to remove
   * @return {@code true} if the set was modified, {@code false} otherwise
   */
  @Override
  default boolean remove(Object o)
  {
    int initialSize = this.size();
    this.discard(o);
    return this.size() != initialSize;
  }

  /**
   * Removes all elements in the specified collection from this set.
   *
   * @param collection the collection of elements to remove
   * @return {@code true} if the set was modified as a result of this operation,
   * {@code false} otherwise
   */
  @Override
  default boolean removeAll(Collection<?> collection)
  {
    int initialSize = this.size();
    PySet delta = this.difference(of(collection));
    this.clear();
    this.update(delta);
    return this.size() != initialSize;
  }

  /**
   * Retains only the elements in this set that are contained in the specified
   * collection.
   *
   * @param collection the collection of elements to retain
   * @return {@code true} if the set was modified as a result of this operation,
   * {@code false} otherwise
   */
  @Override
  default boolean retainAll(Collection<?> collection)
  {
    int initialSize = this.size();
    PySet delta = this.intersect(of(collection));
    this.clear();
    this.update(delta);
    return this.size() != initialSize;
  }

  /**
   * Returns the number of elements in this set.
   *
   * @return the size of the set
   */
  @Override
  public int size();

  /**
   * Returns a {@link Spliterator} for the elements in this set.
   *
   * @return a spliterator for the set
   */
  @Override
  default Spliterator<PyObject> spliterator()
  {
    return Spliterators.spliterator(this, Spliterator.DISTINCT);
  }

  /**
   * Returns a sequential {@link Stream} of the elements in this set.
   *
   * @return a sequential stream of the set's elements
   */
  @Override
  default Stream<PyObject> stream()
  {
    return StreamSupport.stream(this.spliterator(), false);
  }

  /**
   * Returns a new set containing the symmetric difference between this set and
   * the specified sets.
   *
   * @param set the sets to compute the symmetric difference with
   * @return a new {@code PySet} containing the symmetric difference
   */
  PySet symmetricDifference(Collection<?>... set);

  /**
   * Updates this set to contain the symmetric difference between itself and the
   * specified set.
   *
   * @param set the set to compute the symmetric difference with
   */
  void symmetricDifferenceUpdate(Collection<?> set);

  /**
   * Returns an array containing all elements in this set.
   *
   * @return an array containing the set's elements
   */
  @Override
  default Object[] toArray()
  {
    return new ArrayList<>(this).toArray();
  }

  /**
   * Returns an array containing all elements in this set, using the specified
   * array as the target.
   *
   * @param a the array into which the elements of the set are to be stored
   * @param <T> the getType of the array elements
   * @return an array containing the set's elements
   */
  @Override
  default <T> T[] toArray(T[] a)
  {
    return new ArrayList<>(this).toArray(a);
  }

  /**
   * Returns a {@link List} containing all elements in this set, using Python
   * semantics.
   *
   * @return a Python-style list containing the set's elements
   */
  List<PyObject> toList();

  /**
   * Returns a new set containing the union of this set and the specified sets.
   *
   * @param set the sets to compute the union with
   * @return a new {@code PySet} containing the union
   */
  PySet union(Collection<?>... set);

  /**
   * Updates this set to contain the union of itself and the specified sets.
   *
   * @param set the sets to compute the union with
   */
  void unionUpdate(Collection<?>... set);

  /**
   * Updates this set to include all elements from the specified iterable.
   *
   * @param other the iterable providing elements to add to the set
   */
  void update(Iterable<?> other);

}
