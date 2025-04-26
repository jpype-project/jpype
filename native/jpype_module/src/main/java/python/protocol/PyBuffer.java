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

/**
 * Protocol for objects that act as a buffer, providing access to raw data in a
 * Python-like manner.
 *
 * <p>
 * This interface defines the contract for buffer-like objects, enabling
 * operations such as reading, writing, slicing, and inspecting raw data. It is
 * inspired by Python's `memoryview` and `buffer` objects, offering similar
 * functionality in a Java context.
 *
 * <p>
 * Implementations of this interface should provide efficient access to
 * underlying data structures and support common buffer operations.
 *
 * <h2>Suggested Methods</h2>
 * <ul>
 * <li>Accessing raw data</li>
 * <li>Reading and writing data</li>
 * <li>Slicing the buffer</li>
 * <li>Inspecting metadata (e.g., size, format)</li>
 * </ul>
 *
 * @see PyProtocol
 */
public interface PyBuffer extends PyProtocol
{

  /**
   * Returns the size of the buffer in bytes.
   *
   * @return the size of the buffer
   */
  int getSize();

  /**
   * Reads a sequence of bytes from the buffer, starting at the specified
   * offset.
   *
   * @param offset the starting position in the buffer
   * @param length the number of bytes to read
   * @return an array of bytes containing the data
   * @throws IndexOutOfBoundsException if the offset or length is invalid
   */
  byte[] readBytes(int offset, int length);

  /**
   * Writes a sequence of bytes to the buffer, starting at the specified offset.
   *
   * @param offset the starting position in the buffer
   * @param data the array of bytes to write
   * @throws IndexOutOfBoundsException if the offset is invalid or the data
   * exceeds buffer size
   */
  void writeBytes(int offset, byte[] data);

  /**
   * Returns a slice of the buffer as a new {@link PyBuffer} instance.
   *
   * @param offset the starting position of the slice
   * @param length the length of the slice
   * @return a new {@link PyBuffer} representing the sliced portion of the
   * buffer
   * @throws IndexOutOfBoundsException if the offset or length is invalid
   */
  PyBuffer slice(int offset, int length);

  /**
   * Returns the format of the buffer (e.g., "B" for bytes, "I" for integers).
   *
   * @return a string representing the format of the buffer
   */
  String getFormat();

  /**
   * Checks whether the buffer is read-only.
   *
   * @return {@code true} if the buffer is read-only, otherwise {@code false}
   */
  boolean isReadOnly();

  /**
   * Releases the buffer and frees any associated resources.
   *
   * <p>
   * Once the buffer is released, further operations on it may result in
   * undefined behavior.
   */
  void release();
}
