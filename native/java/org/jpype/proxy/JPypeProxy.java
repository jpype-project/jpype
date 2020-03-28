/* ****************************************************************************
  Copyright 2019, Karl Einar Nelson

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 *****************************************************************************/
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import org.jpype.JPypeContext;
import org.jpype.manager.TypeManager;

/**
 *
 * @author Karl Einar Nelson
 */
public class JPypeProxy implements InvocationHandler
{

  JPypeContext context;
  public long instance;
  public long cleanup;
  Class<?>[] interfaces;
  ClassLoader cl = ClassLoader.getSystemClassLoader();

  public static JPypeProxy newProxy(JPypeContext context,
          long instance,
          long cleanup,
          Class<?>[] interfaces)
  {
    JPypeProxy proxy = new JPypeProxy();
    proxy.context = context;
    proxy.instance = instance;
    proxy.interfaces = interfaces;
    proxy.cleanup = cleanup;
    // Proxies must point to the correct class loader.  For most cases the
    // system classloader is find.  But if the class is in a custom classloader
    // we need to use that one instead
    for (Class cls : interfaces)
    {
      ClassLoader icl = cls.getClassLoader();
      if (icl != null && icl != proxy.cl)
        proxy.cl = icl;
    }
    return proxy;
  }

  public Object newInstance()
  {
    Object out = Proxy.newProxyInstance(cl, interfaces, this);
    context.getReferenceQueue().registerRef(out, instance, cleanup);
    return out;
  }

  @Override
  public Object invoke(Object proxy, Method method, Object[] args)
          throws Throwable
  {
    // We can save a lot of effort on the C++ side by doing all the
    // type lookup work here.
    TypeManager typeManager = context.getTypeManager();
    long returnType;
    long[] parameterTypes;
    synchronized (typeManager)
    {
      returnType = typeManager.findClass(method.getReturnType());
      Class<?>[] types = method.getParameterTypes();
      parameterTypes = new long[types.length];
      for (int i = 0; i < types.length; ++i)
      {
        parameterTypes[i] = typeManager.findClass(types[i]);
      }
    }

    return hostInvoke(context.getContext(), method.getName(), instance, returnType, parameterTypes, args);
  }

  private static native Object hostInvoke(long context, String name, long pyObject,
          long returnType, long[] argsTypes, Object[] args);
}
