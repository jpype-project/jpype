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

/**
 * Java front-end interface for the concrete Python `bytes` type.
 *
 * This interface provides methods for creating and manipulating Python `bytes`
 * objects in a Java environment, mimicking Python's `bytes` functionality.
 */
public interface PyBytes extends PyObject, PyBuffer, PySequence<PyInt>
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
   * Decodes a bytes object into a Python string.
   *
   * <p>
   * Values for encoding include:</p>
   * <ul>
   * <li>"utf-8" (default)</li>
   * <li>"ascii"</li>
   * <li>"latin-1"</li>
   * <li>"utf-16"</li>
   * <li>"utf-32"</li>
   * <li>"cp1252" (Windows encoding)</li>
   * </ul>
   *
   * <p>
   * Values for errors include:</p>
   * <ul>
   * <li>"strict" (default): Raises a UnicodeDecodeError for invalid data.</li>
   * <li>"ignore": Ignores invalid characters.</li>
   * <li>"replace": Replaces invalid characters with a replacement character
   * (e.g., ? or �).</li>
   * <li>"backslashreplace": Replaces invalid characters with Python-style
   * escape sequences (e.g., \xNN).</li>
   * <li>"namereplace": Replaces invalid characters with \N{name} escape
   * sequences.</li>
   * <li>"surrogateescape": Uses special surrogate code points for invalid
   * bytes.</li>
   * </ul>
   *
   * @param encoding The character encoding to use for decoding the bytes
   * object. Common values include "utf-8", "ascii", "latin-1", etc or null if
   * not applicable.
   * @param errors An optional argument to specify how to handle errors during
   * decoding. Can be "strict", "ignore", "replace", etc., or null if not
   * applicable.
   * @return a new string resulting from decoding the bytes object.
   */
  PyString decode(CharSequence encoding, CharSequence errors);

  @Override
  default PyInt get(int index)
  {
    return (PyInt) PyBuiltIn.backend().getitemSequence(this, index);
  }

  @Override
  default PyInt remove(int index)
  {
    throw new UnsupportedOperationException("bytes object does not support removal");
  }

  @Override
  default PyInt set(int index, PyInt value)
  {
    throw new UnsupportedOperationException("bytes object does not support assignment");
  }

  /**
   * Gets the length of a bytearray object in bytes.
   *
   * @return the length of the bytearray object, measured in bytes.
   */
  @Override
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
