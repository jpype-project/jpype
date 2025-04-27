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
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 *
 */
public class PyComplexNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
 Interpreter.getInstance().start(new String[0]);
  }
  
  @Test
  public void testCreateComplexNumber()
  {
    double real = 3.0;
    double imag = 4.0;
    PyComplex pyComplex = PyComplex.of(real, imag);

    // Assert that the created PyComplex object is not null
    assertNotNull(pyComplex, "PyComplex object should not be null");

    // Assert that the real and imaginary parts match the input values
    assertEquals(pyComplex.real(), real, "Real part should match the input value");
    assertEquals(pyComplex.imag(), imag, "Imaginary part should match the input value");
  }

  @Test
  public void testType()
  {
    PyType pyType = PyComplex.type();

    // Assert that the PyType object is not null
    assertNotNull(pyType, "PyType object should not be null");
    assertEquals(pyType.getName(), "complex");
  }

  @Test
  public void testRealPart()
  {
    double real = 5.0;
    double imag = 2.0;
    PyComplex pyComplex = PyComplex.of(real, imag);

    // Assert that the real part matches the expected value
    assertEquals(pyComplex.real(), real, "Real part should match the expected value");
  }

  @Test
  public void testImaginaryPart()
  {
    double real = 1.0;
    double imag = 6.0;
    PyComplex pyComplex = PyComplex.of(real, imag);

    // Assert that the imaginary part matches the expected value
    assertEquals(pyComplex.imag(), imag, "Imaginary part should match the expected value");
  }

  @Test
  public void testConjugate()
  {
    double real = 3.0;
    double imag = 4.0;
    PyComplex pyComplex = PyComplex.of(real, imag);

    PyComplex conjugate = pyComplex.conjugate();

    // Assert that the conjugate is not null
    assertNotNull(conjugate, "Conjugate PyComplex object should not be null");

    // Assert that the real part remains the same
    assertEquals(conjugate.real(), real, "Real part of conjugate should match the original real part");

    // Assert that the imaginary part is negated
    assertEquals(conjugate.imag(), -imag, "Imaginary part of conjugate should be negated");
  }

}
