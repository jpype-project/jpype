// --- file: python/lang/PyComplexNGTest.java ---
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

import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 *
 */ 
public class PyComplexNGTest extends PyTestHarness
{


  @Test
  public void testAbs()
  {
    PyComplex a = context.complex(3.0, 4.0);

    PyObject result = a.abs();

    assertTrue(result instanceof PyFloat || result instanceof PyInt);
    assertEquals(((PyNumber) result).toNumber().doubleValue(), 5.0, 0.0001);
  }

  @Test
  public void testAddDouble()
  {
    PyComplex a = context.complex(1.0, 2.0);

    PyObject result = a.add(2.5);

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), 3.5);
    assertEquals(c.imag(), 2.0);
  }

  @Test
  public void testAddLong()
  {
    PyComplex a = context.complex(1.0, 2.0);

    PyObject result = a.add(5L);

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), 6.0);
    assertEquals(c.imag(), 2.0);
  }

  @Test
  public void testAddPyObject()
  {
    PyComplex a = context.complex(1.0, 2.0);
    PyComplex b = context.complex(3.0, 4.0);

    PyObject result = a.add(b);

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), 4.0);
    assertEquals(c.imag(), 6.0);
  }

  @Test
  public void testConjugate()
  {
    double real = 3.0;
    double imag = 4.0;
    PyComplex pyComplex = context.complex(real, imag);

    PyComplex conjugate = pyComplex.conjugate();

    // Assert that the conjugate is not null
    assertNotNull(conjugate, "Conjugate PyComplex object should not be null");

    // Assert that the real part remains the same
    assertEquals(conjugate.real(), real, "Real part of conjugate should match the original real part");

    // Assert that the imaginary part is negated
    assertEquals(conjugate.imag(), -imag, "Imaginary part of conjugate should be negated");
  }

  @Test
  public void testCreateComplexNumber()
  {
    double real = 3.0;
    double imag = 4.0;
    PyComplex pyComplex = context.complex(real, imag);

    // Assert that the created PyComplex object is not null
    assertNotNull(pyComplex, "PyComplex object should not be null");

    // Assert that the real and imaginary parts match the input values
    assertEquals(pyComplex.real(), real, "Real part should match the input value");
    assertEquals(pyComplex.imag(), imag, "Imaginary part should match the input value");
  }

  @Test
  public void testDividePyObject()
  {
    PyComplex a = context.complex(1.0, 2.0);
    PyComplex b = context.complex(3.0, 4.0);

    PyObject result = a.divide(b);

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), 0.44, 0.0001);
    assertEquals(c.imag(), 0.08, 0.0001);
  }

  @Test
  public void testImaginaryPart()
  {
    double real = 1.0;
    double imag = 6.0;
    PyComplex pyComplex = context.complex(real, imag);

    // Assert that the imaginary part matches the expected value
    assertEquals(pyComplex.imag(), imag, "Imaginary part should match the expected value");
  }

  @Test
  public void testMultiplyPyObject()
  {
    PyComplex a = context.complex(1.0, 2.0);
    PyComplex b = context.complex(3.0, 4.0);

    PyObject result = a.multiply(b);

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), -5.0);
    assertEquals(c.imag(), 10.0);
  }

  @Test
  public void testNegate()
  {
    PyComplex zero = context.complex(0.0, 0.0);
    PyComplex nonZero = context.complex(1.0, 1.0);

    assertTrue(zero.negate());
    assertFalse(nonZero.negate());
  }

  @Test
  public void testNegateValue()
  {
    PyComplex a = context.complex(3.0, -4.0);

    PyObject result = a.negateValue();

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), -3.0);
    assertEquals(c.imag(), 4.0);
  }

  @Test
  public void testPositive()
  {
    PyComplex a = context.complex(3.0, -4.0);

    PyObject result = a.positive();

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), 3.0);
    assertEquals(c.imag(), -4.0);
  }

  @Test
  public void testRealPart()
  {
    double real = 5.0;
    double imag = 2.0;
    PyComplex pyComplex = context.complex(real, imag);

    // Assert that the real part matches the expected value
    assertEquals(pyComplex.real(), real, "Real part should match the expected value");
  }

  @Test
  public void testSubtractPyObject()
  {
    PyComplex a = context.complex(5.0, 7.0);
    PyComplex b = context.complex(2.0, 3.0);

    PyObject result = a.subtract(b);

    assertTrue(result instanceof PyComplex);
    PyComplex c = (PyComplex) result;
    assertEquals(c.real(), 3.0);
    assertEquals(c.imag(), 4.0);
  }

  @Test
  public void testToBooleanFalse()
  {
    PyComplex a = context.complex(0.0, 0.0);
    assertFalse(a.toBoolean());
  }

  @Test
  public void testToBooleanTrue()
  {
    PyComplex a = context.complex(1.0, 0.0);
    assertTrue(a.toBoolean());
  }

}
