// --- file: python/lang/PyExcNGTest.java ---
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

import python.exceptions.PyValueError;
import python.exceptions.PyZeroDivisionError;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyExcNGTest extends PyTestHarness
{

  @Test
  public void testCaughtExceptionCarriesPyExc()
  {
    try
    {
      context.eval("1/0");
      fail("Expected a PyZeroDivisionError");
    } catch (PyZeroDivisionError ex)
    {
      PyExc exc = ex.get();
      assertNotNull(exc);
      assertTrue(exc instanceof PyObject);
    }
  }

  @Test
  public void testGetMessage()
  {
    try
    {
      context.eval("int('not a number')");
      fail("Expected a PyValueError");
    } catch (PyValueError ex)
    {
      PyExc exc = ex.get();
      assertNotNull(exc.getMessage());
      assertTrue(exc.getMessage().length() > 0);
    }
  }

  @Test
  public void testUnwrapPyException()
  {
    try
    {
      context.eval("1/0");
      fail("Expected a PyZeroDivisionError");
    } catch (PyZeroDivisionError ex)
    {
      PyExc unwrapped = PyExc.unwrap(ex);
      assertNotNull(unwrapped);
      assertEquals(unwrapped, ex.get());
    }
  }

  @Test
  public void testUnwrapNonPyException()
  {
    assertNull(PyExc.unwrap(new RuntimeException("not a python exception")));
  }

}
