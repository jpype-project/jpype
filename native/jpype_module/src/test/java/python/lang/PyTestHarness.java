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

import org.jpype.Script;
import org.jpype.MainInterpreter;
import org.jpype.internal.NativeLauncherControl;
import org.testng.annotations.*;
import static org.testng.Assert.assertFalse;

/**
 *
 * @author nelson85
 */
public class PyTestHarness
{

  protected static Script context;

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    try
    {

      MainInterpreter interpreter = MainInterpreter.getInstance();
      if (!interpreter.isStarted())
        interpreter.start(new String[0]);
      if (context == null)
        context = new Script(interpreter);
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

  @AfterMethod
  public void checkGilReleased(java.lang.reflect.Method method)
  {
    // Java is guaranteed to have control back here. If the GIL is still
    // held, some call in this test crossed back into Java on a path that
    // didn't release it - a leak invisible to grep, only catchable this way.
    assertFalse(NativeLauncherControl.isGilHeld(),
            "GIL still held on test thread after " + method.getName() + " - leaked GIL acquire somewhere in this test's call path.");
  }

  @AfterSuite(alwaysRun = true)
  public void tearDownBridge()
  {
    System.out.println(">>> Shutting down JPype Bridge...");
    if (MainInterpreter.getInstance().isStarted())
    {
      System.out.println("Close bridge");
      MainInterpreter.getInstance().close();
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
