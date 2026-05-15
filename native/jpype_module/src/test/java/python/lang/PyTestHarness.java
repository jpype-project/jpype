// --- file: python/lang/PyTestHarness.java ---
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

import org.jpype.bridge.Context;
import org.jpype.bridge.Interpreter;
import org.testng.annotations.*;

/**
 *
 * @author nelson85
 */
public class PyTestHarness
{

  protected static Context context;

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    try
    {

      if (!Interpreter.getInstance().isStarted())
        Interpreter.getInstance().start(new String[0]);
      if (context == null)
        context = new Context();
    } catch (Exception ex)
    {
      ex.printStackTrace();
      throw ex;
    }
  }

  @BeforeMethod
  public void logTestStart(java.lang.reflect.Method method)
  {
    // Standard out is often more reliable than loggers during a hard native crash
    System.out.println(">>> RUNNING TEST: " + method.getName());
    System.out.flush();
  }

  @AfterSuite(alwaysRun = true)
  public void tearDownBridge()
  {
    System.out.println(">>> Shutting down JPype Bridge...");
    if (Interpreter.getInstance().isStarted())
    {
      System.out.println("Close bridge");
      Interpreter.getInstance().stop();
      System.out.println("Bridge down");
      try
      {
        System.out.println(context.eval("hello"));
      } catch (Throwable ex)
      {
        System.out.println("Got " + ex);
      }
    }
  }
}
