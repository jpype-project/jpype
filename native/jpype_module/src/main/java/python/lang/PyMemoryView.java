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

import org.jpype.bridge.BuiltIn;
import python.protocol.PyBuffer;

/**
 * Java front-end interface for the Python `memoryview` type.
 * <p>
 * The {@code PyMemoryView} interface represents a concrete Python `memoryview`
 * object, which provides a way to access the memory of other objects (such as
 * bytes or bytearrays) without copying the data. This enables efficient
 * manipulation of binary data in Python.
 * </p>
 *
 * <p>
 * The {@code PyMemoryView} interface extends {@link PyObject}, allowing it to
 * behave as a Python object in a Java environment. It provides access to
 * Python's `memoryview` functionality, enabling developers to work with memory
 * buffers directly from Java.
 * </p>
 *
 * <p>
 * Python's `memoryview` objects support slicing, indexing, and various
 * attributes for inspecting memory buffers. This interface provides methods
 * that align with Python's `memoryview` API to enable efficient manipulation of
 * memory buffers in Java.
 * </p>
 *
 * <p>
 * Example usage:
 * <pre>
 * PyMemoryView memoryView = ...; // Obtain a PyMemoryView instance
 * int length = memoryView.getLength(); // Get the size of the memory buffer
 * byte[] slice = memoryView.getSlice(0, 10); // Get a slice of the memory buffer
 * </pre>
 * </p>
 */
public interface PyMemoryView extends PyObject
{

  /**
   * Returns the Python type object for `memoryview`.
   * <p>
   * This method retrieves the Python `type` object corresponding to the
   * `memoryview` type.
   * </p>
   *
   * @return the {@link PyType} representing the Python `memoryview` type.
   */
  static PyType type()
  {
    return (PyType) BuiltIn.eval("memoryview", null, null);
  }

  /**
   * Returns the size of the memory buffer.
   * <p>
   * This method retrieves the length of the memory buffer represented by the
   * Python `memoryview` object.
   * </p>
   *
   * @return the size of the memory buffer.
   */
  int getLength();

  /**
   * Retrieves the item at the specified index in the memory buffer.
   * <p>
   * This method enables direct access to a specific element in the memory
   * buffer using its index.
   * </p>
   *
   * @param index the index of the item to retrieve.
   * @return the item at the specified index.
   */
  int getItem(int index);

  /**
   * Returns a slice of the memory buffer between the specified indices.
   * <p>
   * This method creates a new {@code PyMemoryView} object representing a subset
   * of the memory buffer.
   * </p>
   *
   * @param start the starting index of the slice.
   * @param end the ending index of the slice.
   * @return a {@code PyMemoryView} representing the slice of the memory buffer.
   */
  PyMemoryView getSlice(int start, int end);

  /**
   * Releases the memory buffer.
   * <p>
   * This method releases the underlying memory buffer, making the
   * {@code PyMemoryView} object unusable. Equivalent to Python's
   * {@code memoryview.release()} method.
   * </p>
   */
  void release();

  /**
   * Checks if the memory buffer is read-only.
   * <p>
   * This method determines whether the memory buffer is read-only. Equivalent
   * to Python's {@code memoryview.readonly} attribute.
   * </p>
   *
   * @return {@code true} if the memory buffer is read-only; {@code false}
   * otherwise.
   */
  boolean isReadOnly();

  /**
   * Returns the format of the elements stored in the memory buffer.
   * <p>
   * This method retrieves the format string that describes the type of elements
   * stored in the memory buffer. Equivalent to Python's
   * {@code memoryview.format} attribute.
   * </p>
   *
   * @return the format string of the memory buffer elements.
   */
  String getFormat();

  /**
   * Returns the shape of the memory buffer.
   * <p>
   * This method retrieves the dimensions of the memory buffer as a tuple.
   * Equivalent to Python's {@code memoryview.shape} attribute.
   * </p>
   *
   * @return a {@link PyTuple} representing the shape of the memory buffer.
   */
  PyTuple getShape();

  /**
   * Returns the strides of the memory buffer.
   * <p>
   * This method retrieves the step sizes to access elements in the memory
   * buffer. Equivalent to Python's {@code memoryview.strides} attribute.
   * </p>
   *
   * @return a {@link PyTuple} representing the strides of the memory buffer.
   */
  PyTuple getStrides();

  /**
   * Returns the sub-offsets of the memory buffer.
   * <p>
   * This method retrieves the sub-offsets of the memory buffer, which are used
   * for multi-dimensional arrays. Equivalent to Python's
   * {@code memoryview.suboffsets} attribute.
   * </p>
   *
   * @return a {@link PyTuple} representing the sub-offsets of the memory
   * buffer.
   */
  PyTuple getSubOffsets();

  /**
   * Retrieves the underlying buffer as a {@link PyBuffer}.
   * <p>
   * This method provides access to the raw memory buffer represented by the
   * {@code PyMemoryView} object.
   * </p>
   *
   * @return the underlying {@link PyBuffer} object.
   */
  PyBuffer getBuffer();

}
