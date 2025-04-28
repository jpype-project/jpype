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
import python.protocol.PyNumber;

/**
 * Java front-end interface for the Python `float` type.
 * 
 * This interface provides functionality for creating and interacting with
 * Python `float` objects in a Java environment, mimicking Python's built-in
 * `float` type.
 *
 * <p>
 * The Python `float` type represents floating-point numbers and supports
 * operations defined for Python numeric types. This interface allows Java
 * developers to work seamlessly with Python `float` objects, bridging the gap
 * between Java's {@code double} type and Python's `float`.
 */
public interface PyFloat extends PyObject, PyNumber
{

  /**
   * Creates a new Python `float` object from the specified Java {@code double}
   * value. The resulting {@link PyFloat} object represents the Python
   * equivalent of the given floating-point number.
   *
   * @param value the {@code double} value to be converted into a Python
   * `float`.
   * @return a new {@link PyFloat} instance representing the Python `float`
   * object.
   */
  static PyFloat of(double value)
  {
    return Interpreter.getBackend().newFloat(value);
  }

}
