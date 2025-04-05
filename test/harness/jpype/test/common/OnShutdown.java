/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  See NOTICE file for details.
**************************************************************************** */
package jpype.common;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 *
 * @author nelson85
 */
public class OnShutdown
{

  static public void addCoverageHook(Object context)
  {
    try
    {
      Method method = context.getClass().getMethod("_addPost", Runnable.class);
      method.invoke(context, new CoverageHook());
    } catch (NoSuchMethodException | SecurityException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex)
    {
      System.out.println("Fail to install post hook " + ex.getMessage());
    }
  }

  private static class CoverageHook implements Runnable
  {

    @Override
    public void run()
    {
      // If coverage tools are being used, we need to dump last before everything
      // shuts down
      try
      {
        Class<?> RT = Class.forName("org.jacoco.agent.rt.RT");
        Method getAgent = RT.getMethod("getAgent");
        Object agent = getAgent.invoke(null);
        Thread.sleep(100);  // make sure we don't clober
        agent.getClass().getMethod("dump", boolean.class).invoke(agent, false);
        System.err.println("*** Coverage dumped");
      } catch (InterruptedException | NullPointerException | ClassNotFoundException | NoSuchMethodException | SecurityException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex)
      {
      }
    }
  }
}
