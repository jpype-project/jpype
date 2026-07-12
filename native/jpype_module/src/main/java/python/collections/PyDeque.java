// --- file: python/collections/PyDeque.java ---
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

import java.util.Collection;
import java.util.Iterator;
import python.lang.PyIterator;
import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code collections.deque} — a
 * double-ended queue with O(1) appends/pops from either end.
 *
 * <p>
 * Create instances via {@link PyCollections#using(python.lang.PyBuiltIn)
 * PyCollections.using(context)}'s {@code deque()} family of factory
 * methods, not directly.
 *
 * <p>
 * {@code deque} is <b>not</b> a Python {@code list} subclass — it doesn't
 * support slicing or O(1) indexed insertion the way {@code list} does — so
 * this interface deliberately does not extend {@link python.lang.PyList} or
 * {@link python.lang.PySequence}. Instead it offers a reasonable, named
 * subset of {@link java.util.Deque}'s shape (
 * {@link #addFirst}/{@link #addLast}/{@link #removeFirst}/
 * {@link #removeLast}/{@link #peekFirst}/{@link #peekLast}/
 * {@link #size}/{@link #isEmpty}/{@link #clear}/{@link #contains}/
 * {@link #iterator}) plus {@code deque}'s own Python-specific vocabulary
 * ({@link #append}, {@link #appendleft}, {@link #pop}, {@link #popleft},
 * {@link #extend}, {@link #extendleft}, {@link #rotate}, {@link #maxlen}).
 */
public interface PyDeque extends PyObject, Iterable<PyObject>
{

  // --- java.util.Deque-shaped subset -----------------------------------
  /**
   * Inserts an element at the front of the deque. Equivalent to
   * {@link #appendleft(PyObject)}.
   *
   * @param e the element to insert.
   */
  void addFirst(PyObject e);

  /**
   * Inserts an element at the end of the deque. Equivalent to
   * {@link #append(PyObject)}.
   *
   * @param e the element to insert.
   */
  void addLast(PyObject e);

  /**
   * Removes and returns the element at the front of the deque. Equivalent
   * to {@link #popleft()}.
   *
   * @return the removed element.
   * @throws RuntimeException (bridged {@code IndexError}) if the deque is
   * empty.
   */
  PyObject removeFirst();

  /**
   * Removes and returns the element at the end of the deque. Equivalent to
   * {@link #pop()}.
   *
   * @return the removed element.
   * @throws RuntimeException (bridged {@code IndexError}) if the deque is
   * empty.
   */
  PyObject removeLast();

  /**
   * @return the element at the front of the deque, or {@code null} if the
   * deque is empty.
   */
  PyObject peekFirst();

  /**
   * @return the element at the end of the deque, or {@code null} if the
   * deque is empty.
   */
  PyObject peekLast();

  /**
   * @return the number of elements currently in the deque.
   */
  int size();

  /**
   * @return {@code true} if the deque has no elements.
   */
  default boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Removes every element from the deque.
   */
  void clear();

  /**
   * @param o the value to search for.
   * @return {@code true} if the deque contains an element equal to
   * {@code o}.
   */
  boolean contains(Object o);

  /**
   * @return a Java {@link Iterator} over the deque's elements, front to
   * back.
   */
  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator<>(builtin().iter(this));
  }

  // --- Python-specific vocabulary ---------------------------------------
  /**
   * Appends a value to the right end of the deque. If the deque has a
   * {@link #maxlen()} and is full, the leftmost element is discarded.
   *
   * @param value the value to append.
   */
  void append(PyObject value);

  /**
   * Appends a value to the left end of the deque. If the deque has a
   * {@link #maxlen()} and is full, the rightmost element is discarded.
   *
   * @param value the value to append.
   */
  void appendleft(PyObject value);

  /**
   * Removes and returns the rightmost element.
   *
   * @return the removed element.
   * @throws RuntimeException (bridged {@code IndexError}) if the deque is
   * empty.
   */
  PyObject pop();

  /**
   * Removes and returns the leftmost element.
   *
   * @return the removed element.
   * @throws RuntimeException (bridged {@code IndexError}) if the deque is
   * empty.
   */
  PyObject popleft();

  /**
   * Appends every element of {@code c}, in order, to the right end of the
   * deque.
   *
   * @param c the elements to append.
   */
  void extend(Collection<? extends PyObject> c);

  /**
   * Appends every element of {@code c} to the left end of the deque. Note
   * this reverses the resulting order, matching Python's
   * {@code deque.extendleft} — each element is individually
   * {@code appendleft}'d.
   *
   * @param c the elements to append.
   */
  void extendleft(Collection<? extends PyObject> c);

  /**
   * Rotates the deque {@code n} steps to the right; negative values rotate
   * to the left. Equivalent to {@code d.appendleft(d.pop())} repeated
   * {@code n} times (or {@code d.append(d.popleft())} for negative
   * {@code n}), but performed in a single O(k) operation.
   *
   * @param n the number of steps to rotate.
   */
  void rotate(int n);

  /**
   * Rotates the deque one step to the right. Equivalent to
   * {@code rotate(1)}.
   */
  default void rotate()
  {
    rotate(1);
  }

  /**
   * @param value the value to count.
   * @return the number of elements in the deque equal to {@code value}.
   */
  int count(Object value);

  /**
   * @param value the value to search for.
   * @return the index of the first element equal to {@code value}.
   * @throws RuntimeException (bridged {@code ValueError}) if not found.
   */
  int index(Object value);

  /**
   * Inserts {@code value} before the element currently at {@code index}.
   *
   * @param index the position to insert before.
   * @param value the value to insert.
   * @throws RuntimeException (bridged {@code IndexError}) if the deque has
   * a {@link #maxlen()} and is already full.
   */
  void insert(int index, PyObject value);

  /**
   * Removes the first element equal to {@code value}.
   *
   * @param value the value to remove.
   * @throws RuntimeException (bridged {@code ValueError}) if not found.
   */
  void remove(Object value);

  /**
   * Reverses the deque in place.
   */
  void reverse();

  /**
   * @return the deque's maximum length, or {@code null} if it is
   * unbounded.
   */
  Integer maxlen();

}
