/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
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
