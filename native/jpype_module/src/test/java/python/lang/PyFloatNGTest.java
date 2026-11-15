// --- file: python/lang/PyFloatNGTest.java ---
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

public class PyFloatNGTest extends PyTestHarness
{

  @Test
  public void testCreate()
  {
    PyFloat a = context.$float(2.5);
    assertNotNull(a);
    assertEquals(a.toNumber().doubleValue(), 2.5, 0.0001);
  }

  @Test
  public void testAddPyObject()
  {
    PyFloat a = context.$float(1.5);
    PyFloat b = context.$float(2.5);
    PyObject result = a.add(b);
    assertTrue(result instanceof PyFloat);
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 4.0, 0.0001);
  }

  @Test
  public void testAddLong()
  {
    PyFloat a = context.$float(1.5);
    PyObject result = a.add(2L);
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 3.5, 0.0001);
  }

  @Test
  public void testSubtract()
  {
    PyFloat a = context.$float(5.5);
    PyObject result = a.subtract(context.$float(2.0));
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 3.5, 0.0001);
  }

  @Test
  public void testMultiply()
  {
    PyFloat a = context.$float(2.5);
    PyObject result = a.multiply(context.$float(2.0));
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 5.0, 0.0001);
  }

  @Test
  public void testDivide()
  {
    PyFloat a = context.$float(5.0);
    PyObject result = a.divide(context.$float(2.0));
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 2.5, 0.0001);
  }

  @Test
  public void testAbs()
  {
    PyFloat a = context.$float(-3.5);
    PyObject result = a.abs();
    assertEquals(((PyFloat) result).toNumber().doubleValue(), 3.5, 0.0001);
  }

  @Test
  public void testNegateValue()
  {
    PyFloat a = context.$float(3.5);
    PyObject result = a.negateValue();
    assertEquals(((PyFloat) result).toNumber().doubleValue(), -3.5, 0.0001);
  }

  @Test
  public void testPositive()
  {
    PyFloat a = context.$float(-3.5);
    PyObject result = a.positive();
    assertEquals(((PyFloat) result).toNumber().doubleValue(), -3.5, 0.0001);
  }

  @Test
  public void testCompareTo()
  {
    PyFloat a = context.$float(5.0);
    assertTrue(a.compareTo(3) > 0);
    assertTrue(a.compareTo(10) < 0);
    assertEquals(a.compareTo(5), 0);
  }

  @Test
  public void testToBooleanTrue()
  {
    assertTrue(context.$float(1.0).toBoolean());
  }

  @Test
  public void testToBooleanFalse()
  {
    assertFalse(context.$float(0.0).toBoolean());
  }

}
