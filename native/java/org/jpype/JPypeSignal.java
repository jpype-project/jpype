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
          interruptPy();
          return null;
        }
      });
      Object intr = Signal.getDeclaredConstructor(String.class).newInstance("INT");
      method.invoke(null, intr, handler);
    } catch (InvocationTargetException | IllegalArgumentException | IllegalAccessException | InstantiationException | ClassNotFoundException | NoSuchMethodException | SecurityException ex)
    {
      // If we don't get the signal handler run without it.  (ANDROID)
    }
  }

  native static void interruptPy();
  native static void acknowledgePy();
}
