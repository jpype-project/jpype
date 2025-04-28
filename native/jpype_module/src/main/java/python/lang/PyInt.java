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
 * Java front-end interface for the Python `int` type.
 *
 * This interface provides functionality for creating and interacting with
 * Python `int` objects in a Java environment, mimicking Python's built-in
 * integer type.
 *
 * <p>
 * The Python `int` type represents arbitrary-precision integers and supports
 * operations defined for Python numeric types. This interface allows Java
 * developers to work seamlessly with Python `int` objects, bridging the gap
 * between Java's {@code long} type and Python's `int`.
 */
public interface PyInt extends PyObject, PyNumber
{

  /**
   * Creates a new Python `int` object from the specified Java {@code long}
   * value. The resulting {@link PyInt} object represents the Python equivalent
   * of the given integer.
   *
   * @param value the {@code long} value to be converted into a Python `int`.
   * @return a new {@link PyInt} instance representing the Python `int` object.
   */
  static PyInt of(long value)
  {
    return Interpreter.getBackend().newInt(value);
  }

}
