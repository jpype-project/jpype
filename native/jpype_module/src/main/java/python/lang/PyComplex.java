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
 * Java front-end interface for the Python `complex` type.
 *
 * This interface provides methods for creating and manipulating Python
 * `complex` numbers in a Java environment.
 */
public interface PyComplex extends PyObject, PyNumber
{

  /**
   * Creates a new Python `complex` number with the specified real and imaginary
   * parts.
   *
   * @param real the real part of the complex number.
   * @param imag the imaginary part of the complex number.
   * @return a new {@link PyComplex} instance representing the complex number.
   */
  static PyComplex of(double real, double imag)
  {
    return Interpreter.getBackend().newComplex(real, imag);
  }

  /**
   * Returns the real part of the complex number.
   *
   * @return the real part as a {@code double}.
   */
  double real();

  /**
   * Returns the imaginary part of the complex number.
   *
   * @return the imaginary part as a {@code double}.
   */
  double imag();

  /**
   * Computes the complex conjugate of the current complex number. The conjugate
   * of a complex number is obtained by negating its imaginary part.
   *
   * @return a new {@link PyComplex} instance representing the conjugate of the
   * current complex number.
   */
  PyComplex conjugate();
}
