// --- file: python/lang/PyByteArray.java ---
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

import org.jpype.annotation.Bypass;

/**
 * Java front-end interface for the concrete Python `bytearray` type.
 *
 * This interface provides methods for creating and manipulating Python
 * bytearrays in a Java environment, mimicking Python's `bytearray`
 * functionality.
 */
public interface PyByteArray extends PyObject, PyBuffer, PySequence<PyInt>
{

  /**
   * Decodes a bytearray object into a Python string.
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
   * @param encoding is the character encoding to use for decoding the bytes
   * object. Common values include "utf-8", "ascii", "latin-1", etc or null if
   * not applicable.
   * @param errors is an optional argument to specify how to handle errors
   * during decoding. Can be "strict", "ignore", "replace", etc., or null if not
   * applicable.
   * @return a new string resulting from decoding the bytearray object.
   */
  PyObject decode(PyObject encoding, PyObject errors);

  @Bypass
  @Override
  default PyInt get(PySubscript... indices)
  {
    throw new IllegalArgumentException("bytearray does not support tuple assignment");
  }

  @Bypass
  @Override
  default PyInt get(PySubscript index)
  {
    return (PyInt) builtin().backend.getitemMappingObject(this, index);
  }

  @Bypass
  @Override
  default PyInt get(int index)
  {
    return (PyInt) builtin().backend.getitemSequence(this, index);
  }

  @Override
  PyInt remove(int index);

  void remove(PySubscript index);

  @Bypass
  @Override
  default PyInt set(int index, PyInt value)
  {
    return (PyInt) builtin().backend.setitemSequence(this, index, value);
  }

  @Bypass
  default void set(PySubscript index, PyObject values)
  {
    builtin().backend.setitemMapping(this, index, values);
  }

  @Bypass
  @Override
  default int size()
  {
    return builtin().backend.len(this);
  }

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
