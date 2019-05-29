package org.jpype.proxy;

import java.lang.reflect.*;

class JPypeInvocationHandler implements InvocationHandler
{
  long context;
  long hostObject;

  public Object invoke(Object proxy, Method method, Object[] args)
  {
    return hostInvoke(context, method.getName(), hostObject, args, method.getParameterTypes(), method.getReturnType());
  }

  private static native Object hostInvoke(long context, String name, long pyObject, Object[] args, Class[] argTypes, Class returnType);
}
