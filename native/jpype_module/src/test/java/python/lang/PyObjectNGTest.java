// --- file: python/lang/PyObjectNGTest.java ---
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

import org.jpype.Script;
import org.jpype.MainInterpreter;
import org.testng.annotations.BeforeClass;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 *
 * @author nelson85
 */
public class PyObjectNGTest extends PyTestHarness
{


  public PyObjectNGTest()
  {
  }



  @Test
  public void testStringProtocol()
  {
    // Assume Interpreter is already started
    PyObject strObj = (PyObject) context.eval("'Hello JPype'");

    // 1. Test toString() mapping to Python str()
    assertEquals("Hello JPype", strObj.toString());

    // 2. Test Attributes Protocol
    PyAttributes attrs = strObj.getAttributes();
    PyObject upperFunc = attrs.get("upper");

    // 3. Test Duck Typing (Check if it's callable)
    assertTrue(upperFunc instanceof PyCallable, "upper should be callable");
//
//    PyObject result = ((PyCallable) upperFunc).call();
//    assertEquals("HELLO JPYPE", result.toString());
  }

  @Test
  public void testIdentity()
  {
    PyObject str1 = (PyObject) context.eval("'A'");
    PyObject str2 = (PyObject) context.eval("'B'");

    // Python identity test
    assertNotEquals(str1, str2, "Distinct Python objects should not be equal");
    assertEquals(str1, str1, "Same object should be equal");

    // Verify hashCode consistency
    int initialHash = str1.hashCode();
    assertEquals(initialHash, str1.hashCode(), "Hash code must be stable");
  }
}
