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
import python.protocol.PyIterator;
import python.protocol.PyIterable;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;

/**
 * Java front-end interface for the Python `list` type.
 *
 * <p>
 * This interface represents a concrete Python `list` object and provides
 * methods that closely align with both the Python `list` API and the Java
 * {@link List} contract. It facilitates seamless integration between Python's
 * dynamic, mutable list behavior and Java's type-safe collections
 * framework.</p>
 *
 * <p>
 * <b>Key Features:</b></p>
 * <ul>
 * <li>Extends {@link PyObject}, {@link List}, and {@link PyIterable}, enabling
 * it to function as both a Python object and a Java collection.</li>
 * <li>Implements standard Java {@link List} methods for compatibility with
 * Java's collections.</li>
 * <li>Provides additional Python-specific methods such as {@code extend},
 * {@code insert}, and {@code addAny} to support Python's list
 * functionality.</li>
 * </ul>
 *
 * <p>
 * <b>Python List Characteristics:</b></p>
 * <ul>
 * <li>Dynamic: Python lists can grow or shrink in size as needed.</li>
 * <li>Mutable: Elements in a Python list can be modified after creation.</li>
 * <li>Heterogeneous: Python lists can contain elements of varying types.</li>
 * </ul>
 *
 * <p>
 * This interface bridges the gap between Python's flexible data structures and
 * Java's statically typed collections, enabling developers to work with Python
 * lists directly in a Java environment.</p>
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
public interface PyList extends PyObject, List<PyObject>, PyIterable
{

  /**
   * Creates a new Python `list` object from the given {@link Iterable}.
   *
   * The elements of the provided iterable will be added to the Python list.
   *
   * @param c the {@link Iterable} whose elements will populate the new Python
   * list.
   * @return a new {@link PyList} instance containing the elements of the
   * iterable.
   */
  public static PyList of(Iterable c)
  {
    return BuiltIn.list(c);
  }

  /**
   * Retrieves the Python type object for `list`. This is equivalent to
   * evaluating `type(list)` in Python.
   *
   * @return the {@link PyType} instance representing the Python `list` type.
   */
  static PyType type()
  {
    return (PyType) BuiltIn.eval("list", null, null);
  }

  /**
   * Adds a Python object to the end of the list.
   *
   * @param e the {@link PyObject} to add.
   * @return {@code true} if the list was modified as a result of this
   * operation.
   */
  @Override
  boolean add(PyObject e);

  /**
   * Inserts a Python object at the specified position in the list.
   *
   * @param index the position at which the object should be inserted.
   * @param element the {@link PyObject} to insert.
   */
  @Override
  void add(int index, PyObject element);

  /**
   * Adds all elements from the specified collection to the end of the list.
   *
   * @param c the collection of {@link PyObject} elements to add.
   * @return {@code true} if the list was modified as a result of this
   * operation.
   */
  @Override
  default boolean addAll(Collection<? extends PyObject> c)
  {
    this.extend(c);
    return !c.isEmpty();
  }

  /**
   * Inserts all elements from the specified collection at the specified
   * position in the list.
   *
   * @param index the position at which the elements should be inserted.
   * @param c the collection of {@link PyObject} elements to insert.
   * @return {@code true} if the list was modified as a result of this
   * operation.
   */
  @Override
  default boolean addAll(int index, Collection<? extends PyObject> c)
  {
    this.insert(index, c);
    return !c.isEmpty();
  }

  /**
   * Adds an arbitrary object to the list. The object will be converted to a
   * Python-compatible type.
   *
   * @param obj the object to add.
   */
  void addAny(Object obj);

  /**
   * Removes all elements from the list.
   */
  @Override
  void clear();

  /**
   * Checks if the list contains the specified object.
   *
   * @param o the object to check for.
   * @return {@code true} if the list contains the object, {@code false}
   * otherwise.
   */
  @Override
  boolean contains(Object o);

  /**
   * Checks if the list contains all elements from the specified collection.
   *
   * @param c the collection of elements to check for.
   * @return {@code true} if the list contains all elements, {@code false}
   * otherwise.
   */
  @Override
  default boolean containsAll(Collection<?> c)
  {
    PySet s1 = PySet.of(this);
    PySet s2 = PySet.of(c);
    return s2.isSubset(s1);
  }

  /**
   * Extends the list by appending all elements from the specified collection.
   *
   * @param c the collection of {@link PyObject} elements to add.
   */
  void extend(Collection<? extends PyObject> c);

  /**
   * Retrieves the element at the specified position in the list.
   *
   * @param index the position of the element to retrieve.
   * @return the {@link PyObject} at the specified position.
   */
  @Override
  PyObject get(int index);

  /**
   * Finds the index of the first occurrence of the specified object in the
   * list.
   *
   * @param o the object to search for.
   * @return the index of the object, or {@code -1} if not found.
   */
  @Override
  int indexOf(Object o);

  /**
   * Inserts a collection of elements at the specified position in the list.
   *
   * @param index the position at which the elements should be inserted.
   * @param c the collection of {@link PyObject} elements to insert.
   */
  void insert(int index, Collection<? extends PyObject> c);

  /**
   * Checks if the list is empty.
   *
   * @return {@code true} if the list is empty, {@code false} otherwise.
   */
  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Returns an iterator over the elements in the list.
   *
   * @return an {@link Iterator} for the list elements.
   */
  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator(this.iter());
  }

  /**
   * Removes the first occurrence of the specified object from the list.
   *
   * @param o the object to remove.
   * @return {@code true} if the object was removed, {@code false} otherwise.
   */
  @Override
  default boolean remove(Object o)
  {
    return Interpreter.getBackend().delByIndex(this, this.indexOf(o));
  }

  /**
   * Removes the element at the specified position in the list.
   *
   * @param index the position of the element to remove.
   * @return the {@link PyObject} that was removed.
   */
  @Override
  default PyObject remove(int index)
  {
    PyObject out = this.get(index);
    Interpreter.getBackend().delByIndex(this, index);
    return out;
  }

  /**
   * Removes all elements in the list that are also contained in the specified
   * collection.
   * <p>
   * This method behaves similarly to {@link List#removeAll(Collection)}, but is
   * implemented in the context of a Python list.
   * </p>
   *
   * @param c the collection containing elements to be removed from the list.
   * @return {@code true} if the list was modified as a result of this
   * operation, {@code false} otherwise.
   */
  @Override
  boolean removeAll(Collection<?> c);

  /**
   * Retains only the elements in the list that are contained in the specified
   * collection.
   * <p>
   * This method behaves similarly to {@link List#retainAll(Collection)}, but is
   * implemented in the context of a Python list.
   * </p>
   *
   * @param c the collection containing elements to be retained in the list.
   * @return {@code true} if the list was modified as a result of this
   * operation, {@code false} otherwise.
   */
  @Override
  boolean retainAll(Collection<?> c);

  /**
   * Replaces the element at the specified position in the list with the
   * specified Python object.
   *
   * @param index the position of the element to replace.
   * @param element the {@link PyObject} to be stored at the specified position.
   * @return the {@link PyObject} previously at the specified position.
   * @throws IndexOutOfBoundsException if the index is out of range.
   */
  @Override
  PyObject set(int index, PyObject element);

  /**
   * Replaces the element at the specified position in the list with an
   * arbitrary object.
   * <p>
   * The object will be converted to a Python-compatible type before being
   * stored.
   * </p>
   *
   * @param index the position of the element to replace.
   * @param obj the object to be stored at the specified position.
   * @throws IndexOutOfBoundsException if the index is out of range.
   */
  void setAny(int index, Object obj);

  /**
   * Returns the number of elements in the list.
   * <p>
   * This method behaves similarly to {@link List#size()}, but is implemented in
   * the context of a Python list.
   * </p>
   *
   * @return the number of elements in the list.
   */
  @Override
  int size();

  /**
   * Returns a view of the portion of the list between the specified
   * {@code fromIndex}, inclusive, and {@code toIndex}, exclusive.
   * <p>
   * The returned sublist is backed by the original list, so changes to the
   * sublist are reflected in the original list and vice versa.
   * </p>
   *
   * @param fromIndex the starting index of the sublist (inclusive).
   * @param toIndex the ending index of the sublist (exclusive).
   * @return a {@link PyList} representing the specified range within the list.
   * @throws IndexOutOfBoundsException if {@code fromIndex} or {@code toIndex}
   * is out of range.
   * @throws IllegalArgumentException if {@code fromIndex > toIndex}.
   */
  @Override
  PyList subList(int fromIndex, int toIndex);

  /**
   * Returns an array containing all elements in the list in proper sequence.
   * <p>
   * This method behaves similarly to {@link List#toArray()}, but is implemented
   * in the context of a Python list.
   * </p>
   *
   * @return an array containing all elements in the list.
   */
  @Override
  default Object[] toArray()
  {
    return new ArrayList<>(this).toArray();
  }

  /**
   * Returns an array containing all elements in the list in proper sequence,
   * using the runtime type of the specified array.
   * <p>
   * If the list fits in the specified array, it is returned therein. Otherwise,
   * a new array is allocated with the runtime type of the specified array and
   * the size of the list.
   * </p>
   *
   * @param a the array into which the elements of the list are to be stored, if
   * it is large enough; otherwise, a new array of the same runtime type is
   * allocated for this purpose.
   * @param <T> the runtime type of the array.
   * @return an array containing all elements in the list.
   * @throws ArrayStoreException if the runtime type of the specified array is
   * not a supertype of the runtime type of every element in the list.
   * @throws NullPointerException if the specified array is {@code null}.
   */
  @Override
  default <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList<>(this).toArray(a);
  }
}
