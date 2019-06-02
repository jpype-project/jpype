/*
 *    Copyright 2019 Karl Einar Nelson
 *   
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 *
 * @author Karl Einar Nelson
 */
public class JPypeProxy
{
  Class<?>[] interfaces;
  long context;
  long instance;

  public static JPypeProxy newProxy(long context, long instance, Class<?>[] interfaces)
  {
    JPypeProxy proxy = new JPypeProxy();
    proxy.context = context;
    proxy.instance = instance;
    proxy.interfaces = interfaces;
    return proxy;
  }

  public Object newInstance()
  {
    ClassLoader cl = ClassLoader.getSystemClassLoader();
    InvocationHandler ih = new InvocationHandler()
    {
      public Object invoke(Object proxy, Method method, Object[] args)
      {
        return hostInvoke(context, method.getName(), instance, args, method.getParameterTypes(), method.getReturnType());
      }
    };
    return Proxy.newProxyInstance(cl, interfaces, ih);
  }

  private static native Object hostInvoke(long context, String name, long pyObject, Object[] args, Class[] argTypes, Class returnType);
}
