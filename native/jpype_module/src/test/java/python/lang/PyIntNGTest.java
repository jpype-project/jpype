// --- file: python/lang/PyIntNGTest.java ---
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

public class PyIntNGTest extends PyTestHarness
{

  @Test
  public void testCreate()
  {
    PyInt a = context.$int(5);
    assertNotNull(a);
    assertEquals(a.toNumber().longValue(), 5L);
  }

  @Test
  public void testAddPyObject()
  {
    PyInt a = context.$int(2);
    PyInt b = context.$int(3);

    PyObject result = a.add(b);

    assertTrue(result instanceof PyInt);
    assertEquals(((PyInt) result).toNumber().longValue(), 5L);
  }

  @Test
  public void testAddLong()
  {
    PyInt a = context.$int(2);
    PyObject result = a.add(3L);
    assertEquals(((PyInt) result).toNumber().longValue(), 5L);
  }

  @Test
  public void testAddDouble()
  {
    PyInt a = context.$int(2);
    PyObject result = a.add(0.5);
    assertTrue(result instanceof PyFloat);
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 2.5, 0.0001);
  }

  @Test
  public void testSubtract()
  {
    PyInt a = context.$int(5);
    PyObject result = a.subtract(context.$int(2));
    assertEquals(((PyInt) result).toNumber().longValue(), 3L);
  }

  @Test
  public void testMultiply()
  {
    PyInt a = context.$int(4);
    PyObject result = a.multiply(context.$int(3));
    assertEquals(((PyInt) result).toNumber().longValue(), 12L);
  }

  @Test
  public void testDivide()
  {
    PyInt a = context.$int(7);
    PyObject result = a.divide(context.$int(2));
    assertEquals(((PyNumber) result).toNumber().doubleValue(), 3.5, 0.0001);
  }

  @Test
  public void testFloorDivide()
  {
    PyInt a = context.$int(7);
    PyObject result = a.floorDivide(context.$int(2));
    assertEquals(((PyInt) result).toNumber().longValue(), 3L);
  }

  @Test
  public void testModulus()
  {
    PyInt a = context.$int(7);
    PyObject result = a.modulus(context.$int(2));
    assertEquals(((PyInt) result).toNumber().longValue(), 1L);
  }

  @Test
  public void testPower()
  {
    PyInt a = context.$int(2);
    PyObject result = a.power(context.$int(10));
    assertEquals(((PyInt) result).toNumber().longValue(), 1024L);
  }

  @Test
  public void testAbs()
  {
    PyInt a = context.$int(-5);
    PyObject result = a.abs();
    assertEquals(((PyInt) result).toNumber().longValue(), 5L);
  }

  @Test
  public void testNegateValue()
  {
    PyInt a = context.$int(5);
    PyObject result = a.negateValue();
    assertEquals(((PyInt) result).toNumber().longValue(), -5L);
  }

  @Test
  public void testCompareTo()
  {
    PyInt a = context.$int(5);
    assertTrue(a.compareTo(3) > 0);
    assertTrue(a.compareTo(10) < 0);
    assertEquals(a.compareTo(5), 0);
  }

  @Test
  public void testToBooleanTrue()
  {
    assertTrue(context.$int(1).toBoolean());
  }

  @Test
  public void testToBooleanFalse()
  {
    assertFalse(context.$int(0).toBoolean());
  }

  @Test
  public void testAddInPlace()
  {
    PyInt a = context.$int(2);
    PyObject result = a.addInPlace(3L);
    assertEquals(((PyInt) result).toNumber().longValue(), 5L);
  }

}
