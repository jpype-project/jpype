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
import java.util.List;
import java.util.ListIterator;
import java.util.Spliterator;
import java.util.Spliterators;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;
import org.jpype.bridge.Interpreter;
import static python.lang.PyBuiltIn.backend;

/**
 * Represents a Java front-end for a concrete Python tuple.
 *
 * <p>
 * A Python tuple is an immutable, ordered collection of elements. This
 * interface provides a Java representation of Python tuples, implementing the
 * {@link List} contract for interoperability with Java collections. However,
 * since Python tuples are immutable, all mutating methods from the {@link List}
 * interface throw {@link UnsupportedOperationException}.
 *
 * <p>
 * Key features:
 * <ul>
 * <li>Immutability: Any attempt to modify the tuple (e.g., add, remove, or
 * replace elements) will result in an
 * {@link UnsupportedOperationException}.</li>
 * <li>Full support for {@link List} methods that do not modify the collection,
 * such as {@code get}, {@code contains}, {@code size}, and
 * {@code iterator}.</li>
 * <li>Integration with Java streams and spliterators for functional
 * programming.</li>
 * </ul>
 *
 * <p>
 * Example usage:
 * <pre>
 * PyTuple tuple = PyTuple.of(1, 2, 3);
 * System.out.println(tuple.get(0)); // Output: 1
 * System.out.println(tuple.size()); // Output: 3
 * </pre>
 *
 * <p>
 * Note: This interface assumes the existence of supporting classes such as
 * {@code BuiltIn}, {@code PyObject}, {@code PyIterable}, {@code PySet}, and
 * {@code PyIterator}.
 *
 * <p>
 * <b>Important Note:</b></p>
 * <p>
 * Python collections are asymmetric in their handling of Java objects. A Java
 * object added to a Python collection will appear as a {@code PyJavaObject}.
 * Developers should exercise caution to avoid reference loops when placing Java
 * objects into Python collections, as this may lead to unintended
 * behaviors.</p>
 *
 */
public interface PyTuple extends PySequence<PyObject>
{

  /**
   * Creates a new {@code PyTuple} from a variable number of elements.
   *
   * @param values the elements to include in the tuple
   * @return a new {@code PyTuple} containing the specified elements
   */
  public static PyTuple of(Object... values)
  {
    return PyBuiltIn.tuple(values);
  }

  /**
   * Creates a new {@code PyTuple} from an {@link Iterable}.
   *
   * @param values an iterator providing the elements to include in the tuple
   * @param <T> the getType of elements in the iterator
   * @return a new {@code PyTuple} containing the elements from the iterator
   */
  public static <T> PyTuple fromItems(Iterable<T> values)
  {
    return PyBuiltIn.tuple(values);
  }

  /**
   * Returns the Python getType object for tuples.
   *
   * @return the Python getType object representing {@code tuple}
   */
  static PyType getType()
  {
    return (PyType) PyBuiltIn.eval("tuple", null, null);
  }

  // --- Mutating methods (throw UnsupportedOperationException) ---
  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Override
  default boolean add(PyObject e)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Override
  default void add(int index, PyObject element)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Override
  default boolean addAll(Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Override
  default boolean addAll(int index, Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Override
  default void clear()
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  // --- Non-mutating methods ---
  /**
   * Checks if the tuple contains the specified object.
   *
   * @param o the object to check
   * @return {@code true} if the tuple contains the object, {@code false}
   * otherwise
   */
  @Override
  default boolean contains(Object obj)
  {
    return backend().contains(this, obj);
  }

  /**
   * Checks if the tuple contains all elements in the specified collection.
   *
   * @param c the collection of elements to check
   * @return {@code true} if the tuple contains all elements in the collection,
   * {@code false} otherwise
   */
  @Override
  default boolean containsAll(Collection<?> c)
  {
    PySet s1 = PySet.of(this);
    PySet s2 = PySet.of(c);
    return s2.isSubset(s1);
  }

  /**
   * Returns the element at the specified index.
   *
   * @param index the index of the element to retrieve.
   * @return the element at the specified index.
   * @throws IndexOutOfBoundsException if the index is out of range
   */
  @Override
  PyObject get(int index);

  /**
   * Returns the index of the first occurrence of the specified object in the
   * tuple.
   *
   * @param o the object to search for
   * @return the index of the first occurrence, or -1 if the object is not found
   */
  @Override
  public int indexOf(Object o);

  /**
   * Returns {@code true} if the tuple is empty.
   *
   * @return {@code true} if the tuple contains no elements, {@code false}
   * otherwise
   */
  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Returns an iterator over the elements in the tuple.
   *
   * @return an iterator over the elements in the tuple
   */
  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator<>(this.iter());
  }

  /**
   * Returns a list iterator over the elements in the tuple.
   *
   * @return a list iterator starting at the beginning of the tuple
   */
  @Override
  default ListIterator<PyObject> listIterator()
  {
    return new PyTupleIterator(this, 0);
  }

  /**
   * Returns a list iterator over the elements in the tuple, starting at the
   * specified index.
   *
   * @param index the starting index for the iterator
   * @return a list iterator starting at the specified index
   * @throws IndexOutOfBoundsException if the index is out of range
   */
  @Override
  default ListIterator<PyObject> listIterator(int index)
  {
    if (index < 0 || index > size())
    {
      throw new IndexOutOfBoundsException();
    }
    return new PyTupleIterator(this, index);
  }

  /**
   * Returns the number of elements in the tuple.
   *
   * @return the number of elements in the tuple
   */
  @Override
  default int size()
  {
    return backend().len(this);
  }

  /**
   * Returns a sublist view of the tuple between the specified indices.
   *
   * @param fromIndex the starting index (inclusive)
   * @param toIndex the ending index (exclusive)
   * @return a sublist view of the tuple
   * @throws IndexOutOfBoundsException if the indices are out of range
   * @throws IllegalArgumentException if {@code fromIndex > toIndex}
   */
  @Override
  PyTuple subList(int fromIndex, int toIndex);

  /**
   * Returns a sequential {@link Stream} over the elements in the tuple.
   *
   * @return a sequential stream of the tuple elements
   */
  @Override
  default Stream<PyObject> stream()
  {
    return StreamSupport.stream(this.spliterator(), false);
  }

  /**
   * Returns a parallel {@link Stream} over the elements in the tuple.
   *
   * @return a parallel stream of the tuple elements
   */
  @Override
  default Stream<PyObject> parallelStream()
  {
    return StreamSupport.stream(this.spliterator(), true);
  }

  /**
   * Returns a {@link Spliterator} over the elements in the tuple.
   *
   * @return a spliterator for the tuple elements
   */
  @Override
  default Spliterator<PyObject> spliterator()
  {
    return Spliterators.spliterator(this, Spliterator.ORDERED);
  }
}
