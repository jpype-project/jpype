package org.jpype;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Java wants to make this action nearly impossible.
 *
 * Thus the have warnings against it that cannot be disabled. So we will skin
 * this cat another way.
 */
public class JPypeSignal
{

  static Thread main;

  static void installHandlers()
  {
    try
    {
      Class Signal = Class.forName("sun.misc.Signal");
      Class SignalHandler = Class.forName("sun.misc.SignalHandler");
      main = Thread.currentThread();
      Method method = Signal.getMethod("handle", Signal, SignalHandler);

      Object handler = Proxy.newProxyInstance(ClassLoader.getSystemClassLoader(), new Class[]
      {
        SignalHandler
      }, new InvocationHandler()
      {
        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable
        {
          main.interrupt();
          return null;
        }
      });
      Object intr = Signal.getDeclaredConstructor(String.class).newInstance("INT");
      method.invoke(null, intr, handler);
    } catch (InvocationTargetException | IllegalArgumentException | IllegalAccessException | InstantiationException | ClassNotFoundException | NoSuchMethodException | SecurityException ex)
    {
      throw new RuntimeException(ex);
    }
  }
}
