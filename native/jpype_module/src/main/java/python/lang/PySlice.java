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

/**
 * Java front-end interface for the Python `slice` type.
 * <p>
 * The {@code PySlice} interface represents a concrete Python `slice` object,
 * which is used to define a range of indices for slicing sequences in Python.
 * </p>
 *
 * <p>
 * The {@code PySlice} interface extends {@link PyObject} and {@link PyIndex},
 * allowing it to behave as both a Python object and an indexable object in a
 * Java environment.
 * </p>
 *
 * <p>
 * Python's `slice` objects support attributes for inspecting the slicing
 * parameters and methods for applying the slice to sequences. This interface
 * provides methods that align with Python's `slice` API to enable efficient
 * manipulation of slices in Java.
 * </p>
 *
 * <p>
 * Example usage:
 * <pre>
 * PySlice slice = ...; // Obtain a PySlice instance
 * Integer start = slice.getStart(); // Get the starting index
 * Integer stop = slice.getStop(); // Get the stopping index
 * Integer step = slice.getStep(); // Get the step size
 * PyTuple indices = slice.indices(10); // Get normalized indices for a sequence of length 10
 * PySequence result = slice.apply(sequence); // Apply the slice to a sequence
 * </pre>
 * </p>
 */
public interface PySlice extends PyObject, PyIndex
{

  /**
   * Returns the starting index of the slice.
   * <p>
   * This method retrieves the {@code start} parameter of the slice, equivalent
   * to Python's {@code slice.start} attribute.
   * </p>
   *
   * @return the starting index of the slice, or {@code null} if not specified.
   */
  Integer getStart();

  /**
   * Returns the stopping index of the slice.
   * <p>
   * This method retrieves the {@code stop} parameter of the slice, equivalent
   * to Python's {@code slice.stop} attribute.
   * </p>
   *
   * @return the stopping index of the slice, or {@code null} if not specified.
   */
  Integer getStop();

  /**
   * Returns the step size of the slice.
   * <p>
   * This method retrieves the {@code step} parameter of the slice, equivalent
   * to Python's {@code slice.step} attribute.
   * </p>
   *
   * @return the step size of the slice, or {@code null} if not specified.
   */
  Integer getStep();

  /**
   * Returns normalized indices for a sequence of the specified length.
   * <p>
   * This method calculates the normalized {@code start}, {@code stop}, and
   * {@code step} values for slicing a sequence of the given length, equivalent
   * to Python's {@code slice.indices(length)} method.
   * </p>
   *
   * @param length the length of the sequence to normalize indices for.
   * @return a {@link PyTuple} containing the normalized {@code start},
   * {@code stop}, and {@code step} values.
   */
  PyTuple indices(int length);

  /**
   * Validates the slice parameters.
   * <p>
   * This method checks whether the slice parameters ({@code start},
   * {@code stop}, {@code step}) are logically consistent. For example,
   * {@code step} cannot be {@code 0}.
   * </p>
   *
   * @return {@code true} if the slice parameters are valid; {@code false}
   * otherwise.
   */
  boolean isValid();

}
