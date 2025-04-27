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
package python.protocol;

import java.util.Iterator;
import org.jpype.bridge.BuiltIn;
import python.lang.PyObject;

/**
 * Java front end for the abstract concept of a Python iterable.
 *
 * <p>
 * This interface provides methods that mimic Python's built-in operations for
 * iterable objects, enabling seamless integration of Python-like functionality
 * into Java code.
 *
 * <p>
 * Note: The `reversed` method was removed due to contract conflicts with
 * {@link List}.
 *
 */
public interface PyIterable extends PyProtocol, Iterable<PyObject>
{

  /**
   * Checks if all elements in the iterable evaluate to {@code true}.
   *
   * @return {@code true} if all elements are true, otherwise {@code false}
   */
  boolean allMatch();

  /**
   * Checks if any element in the iterable evaluates to {@code true}.
   *
   * @return {@code true} if at least one element is true, otherwise
   * {@code false}
   */
  boolean anyMatch();

  /**
   * Returns a Python-style iterator for this iterable.
   *
   * @return a {@link PyIter} instance for this iterable
   */
  default PyIter iter()
  {
    return BuiltIn.iter(this);
  }

  /**
   * Provides a Java {@link Iterator} implementation for this iterable.
   *
   * @return a Java iterator for this iterable
   */
  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator(this.iter());
  }

  /**
   * Applies the given callable function to each element in the iterable and
   * returns a new iterable containing the results.
   *
   * @param callable the function to apply to each element
   * @return a new iterable containing the results of the function application
   */
  PyObject mapElements(PyCallable callable);

  /**
   * Returns the maximum element in the iterable.
   *
   * @return the maximum element
   */
  PyObject findMax();

  /**
   * Returns the minimum element in the iterable.
   *
   * @return the minimum element
   */
  PyObject findMin();

  /**
   * Returns a new iterable containing the elements of this iterable in sorted
   * order.
   *
   * @return a new iterable with sorted elements
   */
  PyObject getSorted();

  /**
   * Computes the sum of all elements in the iterable.
   *
   * @return the sum of the elements
   */
  PyObject computeSum();
}
