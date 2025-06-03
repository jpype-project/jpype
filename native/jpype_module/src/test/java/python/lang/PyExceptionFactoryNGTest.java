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
 * @author nelson85
 */
public class PyExceptionFactoryNGTest
{
  
  public PyExceptionFactoryNGTest()
  {
  }


  @BeforeClass
  public static void setUpClass() throws Exception
  {
 Interpreter.getInstance().start(new String[0]);
  }
  

  @Test
  public void testSomeMethod()
  {
    fail("The test case is a prototype.");
  }
  
}
