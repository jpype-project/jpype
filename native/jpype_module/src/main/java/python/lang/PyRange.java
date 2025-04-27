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

import python.protocol.PyGenerator;
import org.jpype.bridge.BuiltIn;
import python.protocol.PyIter;
import python.protocol.PyIterator;

/**
 * Java front-end interface for the Python `range` type.
 * <p>
 * The {@code PyRange} interface represents a concrete Python `range` object,
 * which is an immutable sequence of numbers commonly used for iteration in
 * Python.
 * </p>
 *
 * <p>
 * The {@code PyRange} interface extends {@link PyObject} and
 * {@link PyGenerator}, allowing it to behave as both a Python object and a
 * generator in a Java environment. This enables developers to work with
 * Python's `range` functionality directly from Java.
 * </p>
 *
 * <p>
 * Python's `range` objects support iteration, slicing, and various attributes
 * for inspecting the sequence. This interface provides methods that align with
 * Python's `range` API, enabling efficient manipulation of ranges in Java.
 * </p>
 *
 * <p>
 * Example usage:
 * <pre>
 * PyRange range = BuiltIn.range(1,5); // Obtain a PyRange instance
 * int length = range.getLength(); // Get the number of elements in the range
 * int first = range.getItem(0); // Get the first item in the range
 * PyRange slice = range.getSlice(1, 5); // Get a slice of the range
 * boolean containsValue = range.contains(3); // Check if the range contains a value
 * PyList rangeAsList = range.toList(); // Convert the range to a list
 * </pre>
 * </p>
 */
public interface PyRange extends PyIter
{

  /**
   * Returns the Python type object for `range`.
   * <p>
   * This method retrieves the Python {@code type} object corresponding to the
   * `range` type.
   * </p>
   *
   * @return the {@link PyType} representing the Python `range` type.
   */
  static PyType type()
  {
    return (PyType) BuiltIn.eval("range", null, null);
  }

  /**
   * Returns the starting value of the range.
   * <p>
   * This method retrieves the first value in the range, equivalent to Python's
   * {@code range.start} attribute.
   * </p>
   *
   * @return the starting value of the range.
   */
  int getStart();

  /**
   * Returns the stopping value of the range.
   * <p>
   * This method retrieves the end value of the range, equivalent to Python's
   * {@code range.stop} attribute.
   * </p>
   *
   * @return the stopping value of the range.
   */
  int getStop();

  /**
   * Returns the step value of the range.
   * <p>
   * This method retrieves the step size between consecutive values in the
   * range, equivalent to Python's {@code range.step} attribute.
   * </p>
   *
   * @return the step value of the range.
   */
  int getStep();

  /**
   * Returns the number of elements in the range.
   * <p>
   * This method calculates the total number of values in the range, equivalent
   * to Python's {@code len(range)} operation.
   * </p>
   *
   * @return the number of elements in the range.
   */
  int getLength();

  /**
   * Retrieves the item at the specified index in the range.
   * <p>
   * This method provides access to a specific element in the range using its
   * index, equivalent to Python's {@code range[index]} operation.
   * </p>
   *
   * @param index the index of the item to retrieve.
   * @return the item at the specified index.
   * @throws IndexOutOfBoundsException if the index is out of range.
   */
  int getItem(int index);

  /**
   * Returns a slice of the range between the specified indices.
   * <p>
   * This method creates a new {@code PyRange} object representing a subset of
   * the original range, equivalent to Python's {@code range[start:end]} slicing
   * operation.
   * </p>
   *
   * @param start the starting index of the slice.
   * @param end the ending index of the slice.
   * @return a {@code PyRange} representing the slice of the range.
   * @throws IllegalArgumentException if the indices are invalid.
   */
  PyRange getSlice(int start, int end);

  /**
   * Checks if the specified value exists in the range.
   * <p>
   * This method determines whether the given value is part of the range,
   * equivalent to Python's {@code value in range} operation.
   * </p>
   *
   * @param value the value to check for.
   * @return {@code true} if the value exists in the range; {@code false}
   * otherwise.
   */
  boolean contains(int value);

  // FIXME MOVE TO PyIter
  /**
   * Converts the range object into a list of integers.
   * <p>
   * This method creates a {@link PyList} containing all the elements in the
   * range, equivalent to Python's {@code list(range)} operation.
   * </p>
   *
   * @return a {@link PyList} containing all the elements in the range.
   */
  PyList toList();

  // FIXME MOVE TO PyIter
  /**
   * Returns an iterator for the range object.
   * <p>
   * This method provides an iterator to traverse the elements of the range,
   * equivalent to Python's {@code iter(range)} operation.
   * </p>
   *
   * @return a {@link PyIterator} for the range object.
   */
  @Override
  PyIterator iterator();

}
