// --- file: python/lang/PyTuple.java ---
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
import org.jpype.annotation.Bypass;

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


  // --- Mutating methods (throw UnsupportedOperationException) ---
  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Bypass
  @Override
  default boolean add(PyObject e)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Bypass
  @Override
  default void add(int index, PyObject element)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Bypass
  @Override
  default boolean addAll(Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Bypass
  @Override
  default boolean addAll(int index, Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }

  /**
   * Throws {@link UnsupportedOperationException} because {@code PyTuple} is
   * immutable.
   */
  @Bypass
  @Override
  default void clear()
  {
    throw new UnsupportedOperationException("PyTuple is immutable.");
  }


  // --- Non-mutating methods ---
  /**
   * Checks if the tuple contains the specified object.
   *
   * @param obj the object to check
   * @return {@code true} if the tuple contains the object, {@code false}
   * otherwise
   */
  @Bypass
  @Override
  default boolean contains(Object obj)
  {
    return builtin().backend.contains(this, obj);
  }

  /**
   * Checks if the tuple contains all elements in the specified collection.
   *
   * @param c the collection of elements to check
   * @return {@code true} if the tuple contains all elements in the collection,
   * {@code false} otherwise
   */
  @Bypass
  @Override
  default boolean containsAll(Collection<?> c)
  {
    PySet s1 = builtin().set(this);
    PySet s2 = builtin().set(c);
    return s2.isSubset(s1);
  }

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
  @Bypass
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
  @Bypass
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
  @Bypass
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
  @Bypass
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
   * Returns a parallel {@link Stream} over the elements in the tuple.
   *
   * @return a parallel stream of the tuple elements
   */
  @Bypass
  @Override
  default Stream<PyObject> parallelStream()
  {
    return StreamSupport.stream(this.spliterator(), true);
  }

  /**
   * Returns the number of elements in the tuple.
   *
   * @return the number of elements in the tuple
   */
  @Bypass
  @Override
  default int size()
  {
    return builtin().backend.len(this);
  }

  /**
   * Returns a {@link Spliterator} over the elements in the tuple.
   *
   * @return a spliterator for the tuple elements
   */
  @Bypass
  @Override
  default Spliterator<PyObject> spliterator()
  {
    return Spliterators.spliterator(this, Spliterator.ORDERED);
  }

  /**
   * Returns a sequential {@link Stream} over the elements in the tuple.
   *
   * @return a sequential stream of the tuple elements
   */
  @Bypass
  @Override
  default Stream<PyObject> stream()
  {
    return StreamSupport.stream(this.spliterator(), false);
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

}
