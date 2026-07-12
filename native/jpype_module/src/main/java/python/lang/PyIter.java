// --- file: python/lang/PyIter.java ---
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

import java.util.Iterator;
import java.util.NoSuchElementException;
import org.jpype.MainInterpreter;
import org.jpype.annotation.Bypass;

/**
 * Represents the Python concept of an iterator in Java.
 * <p>
 * This interface provides methods to work with Python-style iterators in a Java
 * environment. It can be converted to a Java {@link java.util.Iterator} by
 * calling {@link #iterator()}.
 * </p>
 *
 * @param <T> the type of elements returned by this iterator.
 */
public interface PyIter<T> extends PyObject
{

  /**
   * Duplicate a Python iterator.
   *
   * Creates a duplicate of this iterator, allowing the original iterator and
   * the duplicate to be used independently. Both iterators will share the same
   * underlying data, but changes to the state of one iterator (e.g., advancing
   * its position) will not affect the other.
   * <p>
   * This method is uses Python {@code itertools.tee()} function, which enables
   * the creation of multiple independent iterators from a single iterator. It
   * is particularly useful in scenarios where the same data needs to be
   * iterated over multiple times or processed in parallel.
   * </p>
   *
   * <h3>Usage Example:</h3>
   * <pre>
   * PyIter<String> original = PyList.of(1,2,3).iter(); // Obtain the original iterator
   * PyIter<String> duplicate = original.tee();
   *
   * // Use both iterators independently
   * String fromOriginal = original.next();
   * String fromDuplicate = duplicate.next();
   * </pre>
   *
   * <p>
   * <strong>Note:</strong> The behavior of this method depends on the
   * underlying implementation of the iterator. Some iterators may not support
   * duplication or may impose restrictions on how the duplicate can be used.
   * For example, iterators consuming streaming data may require buffering to
   * ensure independent iteration.
   * </p>
   *
   * @return a new {@link PyIter} instance that is an independent duplicate of
   * this iterator.
   * @throws RuntimeException if the iterator does not support duplication.
   */
  PyIter<T> tee();

  /**
   * Filters the elements of the iterator based on the provided callable
   * function.
   * <p>
   * The callable should return a boolean value indicating whether an element
   * should be included in the filtered iterator.
   * </p>
   *
   * @param callable the {@link PyCallable} object representing the filtering
   * function.
   * @return a new {@link PyIter} containing only the elements that satisfy the
   * filtering condition.
   */
  PyIter<T> filter(PyCallable callable);

  /**
   * Converts the Python iterator into a standard Java
   * {@link java.util.Iterator}.
   * <p>
   * This allows the Python-style iterator to be used in Java's enhanced
   * {@code for} loops and other iteration constructs.
   * </p>
   *
   * @return a {@link java.util.Iterator} instance that wraps the Python
   * iterator.
   * @implNote The implementation uses {@link PyIterator} to wrap the Python
   * iterator. The decision to "tee" (duplicate) the iterator has not been
   * finalized.
   */
    @Bypass
  default Iterator<T> iterator()
  {
    // It is not clear if we should tee the iterator here or not.
    //   return new PyIterator(backend.tee(this));    
    return new PyIterator<>(this);
  }

  /**
   * Retrieves the next element in the iterator.
   * <p>
   * If there are no more elements, a {@link java.util.NoSuchElementException}
   * is thrown.
   * </p>
   *
   * @return the next element in the iterator.
   * @throws NoSuchElementException if the iterator has no more elements.
   * @implNote Internally, this method calls
   * {@code backend.next(this, Interpreter.stop)} to fetch the next element.
   * If the backend returns the special {@code Interpreter.stop} object, it
   * indicates the end of the iteration.
   */
  @SuppressWarnings("unchecked")
  @Bypass
  default T next()
  {
    PyObject out =  builtin().backend.next(this, MainInterpreter.stop);
    if (out.equals(MainInterpreter.stop))
      throw new NoSuchElementException();
    return (T) out;
  }

  /**
   * Retrieves the next element in the iterator, or returns the provided default
   * value if the iterator has no more elements.
   *
   * @param defaults the {@link PyObject} to return when the iterator is
   * exhausted.
   * @return the next element in the iterator, or the provided default value if
   * the iterator has no more elements.
   * @implNote Internally, this method calls
   * {@code backend.next(this, defaults)} to fetch the next element.
   */
    @Bypass
    @SuppressWarnings("unchecked")
  default T next(PyObject defaults)
  {
    return (T) builtin().backend.next(this, defaults);
  }

  /**
   * Retrieves all remaining items from the iterator as a list.
   *
   * This method consumes all remaining items from the iterator and returns them
   * in a PyList, similar to Python's `list(iter(...))`.
   *
   * @return a PyList containing all remaining items.
   */
  PyList toList();

  /**
   * Retrieves all remaining items from the iterator as a set.
   *
   * This method consumes all remaining items from the iterator and returns them
   * in a PyList, similar to Python's `set(iter(...))`.
   *
   * @return a PySet containing all remaining items.
   */
  PyList toSet();

}
