// --- file: python/lang/PyExceptionFactoryNGTest.java ---
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

import python.exceptions.PyKeyError;
import python.exceptions.PyTypeError;
import python.exceptions.PyValueError;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 *
 * @author nelson85
 */
public class PyExceptionFactoryNGTest extends PyTestHarness
{

  public PyExceptionFactoryNGTest()
  {
  }

  @Test
  public void testLookupMapsPythonNamesToJavaExceptions()
  {
    assertEquals(PyExceptionFactory.LOOKUP.get("KeyError"), PyKeyError.class);
    assertEquals(PyExceptionFactory.LOOKUP.get("TypeError"), PyTypeError.class);
    assertEquals(PyExceptionFactory.LOOKUP.get("ValueError"), PyValueError.class);
  }

  @Test
  public void testLookupHasNoEntryForUnknownName()
  {
    assertNull(PyExceptionFactory.LOOKUP.get("NotARealPythonException"));
  }

}
