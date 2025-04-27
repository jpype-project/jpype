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

import org.jpype.bridge.Interpreter;
import python.protocol.PyBuffer;

/**
 * Java front-end interface for the concrete Python `bytes` type.
 *
 * This interface provides methods for creating and manipulating Python `bytes`
 * objects in a Java environment, mimicking Python's `bytes` functionality.
 */
public interface PyBytes extends PyObject, PyBuffer
{

  /**
   * Creates a new Python `bytes` object with a fixed length. The `bytes` object
   * will be initialized with zero bytes.
   *
   * @param length the length of the `bytes` object to create.
   * @return a new {@link PyBytes} instance with the specified length.
   */
  static PyBytes create(int length)
  {
    return Interpreter.getBackend().newBytesOfSize(length);
  }

  /**
   * Decodes the contents of the `bytes` object using the specified encoding.
   *
   * Optionally, specific bytes can be deleted during decoding.
   *
   * @param encoding the encoding to use for decoding (e.g., "utf-8").
   * @param delete the bytes to delete during decoding, or {@code null} for no
   * deletion.
   * @return a {@link PyObject} representing the decoded string.
   */
  static PyBytes fromHex(CharSequence str)
  {
    return Interpreter.getBackend().bytesFromHex(str);
  }

  /**
   * Creates a new Python `bytes` object from an iterable of Python objects.
   *
   * Each object in the iterable must be convertible to a byte.
   *
   * @param iterable the iterable containing {@link PyObject} instances to
   * include in the `bytes` object.
   * @return a new {@link PyBytes} instance containing the bytes derived from
   * the iterable.
   */
  static PyByteArray of(Iterable<PyObject> iterable)
  {
    return Interpreter.getBackend().newByteArrayFromIterator(iterable);
  }

  /**
   * Creates a new Python `bytes` object from a {@link PyBuffer}.
   *
   * The buffer's contents will be used to initialize the `bytes` object.
   *
   * @param buffer the {@link PyBuffer} containing the data to populate the
   * `bytes` object.
   * @return a new {@link PyBytes} instance containing the bytes from the
   * buffer.
   */
  static PyByteArray of(PyBuffer buffer)
  {
    return Interpreter.getBackend().newByteArrayFromBuffer(buffer);
  }

  /**
   * Retrieves the Python type object for `bytes`.
   *
   * This is equivalent to evaluating `type(bytes)` in Python.
   *
   * @return the {@link PyType} instance representing the Python `bytes` type.
   */
  static PyType type()
  {
    return (PyType) PyBuiltIn.eval("bytes", null, null);
  }

  PyObject decode(PyObject string, PyObject encoding);

  default int size()
  {
    return Interpreter.getBackend().len(this);
  }

  /**
   * Translates the contents of the `bytes` object using a translation table.
   *
   * The translation table maps byte values to their replacements.
   *
   * @param table the translation table as a {@link PyObject}, where each byte
   * value is mapped to its replacement.
   * @return a new {@link PyObject} representing the translated `bytes` object.
   */
  PyObject translate(PyObject table);
}
