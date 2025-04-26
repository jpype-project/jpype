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
import org.jpype.bridge.BuiltIn;
import python.protocol.PyBuffer;

/**
 * Java front-end interface for the concrete Python `bytearray` type.
 *
 * This interface provides methods for creating and manipulating Python
 * bytearrays in a Java environment, mimicking Python's `bytearray`
 * functionality.
 */
public interface PyByteArray extends PyObject
{

  /**
   * Creates a new Python bytearray with a fixed length. The bytearray will be
   * initialized with zero bytes.
   *
   * @param length the length of the bytearray to create.
   * @return a new {@link PyByteArray} instance with the specified length.
   */
  static PyByteArray create(int length)
  {
    return Interpreter.getBackend().newByteArrayOfSize(length);
  }

  /**
   * Creates a new Python bytearray from an iterable of Python objects. Each
   * object in the iterable must be convertible to a byte.
   *
   * @param iter the iterable containing {@link PyObject} instances to include
   * in the bytearray.
   * @return a new {@link PyByteArray} instance containing the bytes derived
   * from the iterable.
   */
  static PyByteArray of(Iterable<PyObject> iter)
  {
    return Interpreter.getBackend().newByteArrayFromIterable(iter);
  }

  /**
   * Creates a new Python bytearray from a {@link PyBuffer}. The buffer's
   * contents will be used to initialize the bytearray.
   *
   * @param bytes the {@link PyBuffer} containing the data to populate the
   * bytearray.
   * @return a new {@link PyByteArray} instance containing the bytes from the
   * buffer.
   */
  static PyByteArray of(PyBuffer bytes)
  {
    return Interpreter.getBackend().newByteArrayFromBuffer(bytes);
  }

  /**
   * Retrieves the Python type object for `bytearray`. This is equivalent to
   * evaluating `type(bytearray)` in Python.
   *
   * @return the {@link PyType} instance representing the Python `bytearray`
   * type.
   */
  static PyType type()
  {
    return (PyType) BuiltIn.eval("bytearray", null, null);
  }

  /**
   * Creates a new Python bytearray from a hexadecimal string. The input string
   * must contain valid hexadecimal characters.
   *
   * @param str the hexadecimal string to convert into a bytearray.
   * @return a new {@link PyByteArray} instance containing the bytes represented
   * by the hex string.
   */
  static PyByteArray fromHex(CharSequence str)
  {
    return Interpreter.getBackend().bytearrayFromHex(str);
  }

  /**
   * Decodes the contents of the bytearray using the specified encoding.
   * Optionally, specific bytes can be deleted during decoding.
   *
   * @param encoding the encoding to use for decoding (e.g., "utf-8").
   * @param delete the bytes to delete during decoding, or {@code null} for no
   * deletion.
   * @return a {@link PyObject} representing the decoded string.
   */
  PyObject decode(PyObject encoding, PyObject delete);

  /**
   * Translates the contents of the bytearray using a translation table. The
   * translation table maps byte values to their replacements.
   *
   * @param table the translation table as a {@link PyObject}, where each byte
   * value is mapped to its replacement.
   * @return a new {@link PyObject} representing the translated bytearray.
   */
  PyObject translate(PyObject table);

}
