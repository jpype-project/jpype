// --- file: org/jpype/proxy/JPypeProxyInstance.java ---
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import org.jpype.JPypeContext;

public class JPypeProxyInstance implements InvocationHandler
{

  private final JPypeProxyType type;
  private final long instance;
  public static Object missing = new Object();

  public JPypeProxyInstance(JPypeProxyType type, long instance)
  {
    this.type = type;
    this.instance = instance;
  }

  @Override
  public Object invoke(Object proxy, Method method, Object[] args) throws Throwable
  {
    JPypeContext context = JPypeContext.getInstance();
    if (context.isShutdown())
      throw new RuntimeException("Proxy called during shutdown");

    JPypeMethodDescriptor md = type.getMethodDescriptor(method);

    // Resolve method parameter and return types
    // The type resolution logic remains, but uses the shared context
    Object result = hostInvoke(md.name, instance, md.returnType, md.parameterTypes, args, missing);

    if (result != missing)
      return result;

    // FIXME in Java 16 they made it possible to call Default, once we abandon 9 we can safely run it.
    //    return InvocationHandler.invokeDefault(proxy, method, args);
    // Handle default methods in interfaces
    if (md.defaultHandler != null)
      return md.defaultHandler.bindTo(proxy).invokeWithArguments(args);

    throw new NoSuchMethodError(method.getName());
  }

  // exports to JNI
  private static long getInstance(Object obj)
  {
    if (obj == null || !Proxy.isProxyClass(obj.getClass()))
      return 0L;
    InvocationHandler handler = Proxy.getInvocationHandler(obj);
    if (handler instanceof JPypeProxyInstance)
      return ((JPypeProxyInstance) handler).instance;
    return 0L;
  }

  /**
   * Native method to invoke a method on the Python object.
   *
   * @param name is the name of the method to invoke.
   * @param pyObject is the instance ID of the Python object.
   * @param returnType is the return type of the method.
   * @param argsTypes is the types of the method parameters.
   * @param args is the arguments passed to the method.
   * @param bad is the object indicating a missing implementation.
   * @return the result of the method invocation.
   */
  private static native Object hostInvoke(String name, long pyObject,
          long returnType, long[] argsTypes, Object[] args, Object bad);

}
