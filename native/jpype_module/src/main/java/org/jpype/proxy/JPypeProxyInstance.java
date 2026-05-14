// --- file: org/jpype/proxy/JPypeProxyInstance.java ---
package org.jpype.proxy;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import org.jpype.JPypeContext;
import org.jpype.manager.TypeManager;

public class JPypeProxyInstance implements InvocationHandler
{

  static final TypeManager manager = JPypeContext.getInstance().getTypeManager();
  private final JPypeProxyType type;
  private final long instance;
  
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

    // Set up to transfer all the types on the downcall
    int sz = args.length;
    long[] scratch = get(sz);
    for (int i = 0; i < sz; ++i)
    {
      long cls = manager.findClassForObject(args[i]);
      if (cls == 0L)
        cls = md.parameterTypes[i];
      scratch[i] = cls;
    }

    // Resolve method parameter and return types
    // The type resolution logic remains, but uses the shared context
    Object result = hostInvoke(md.name, instance, md.returnType, scratch, args, sz);

    if (result != scratch)
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

  private static final int INITIAL_SIZE = 16;

  private static final ThreadLocal<long[]> CACHE = ThreadLocal.withInitial(() -> new long[INITIAL_SIZE]);

  /**
   * Ensures the current thread's cache is at least 'requiredSize'. Returns the
   * (possibly new) array.
   */
  public static long[] get(int requiredSize)
  {
    long[] current = CACHE.get();
    if (current.length < requiredSize)
    {
      long[] next = new long[requiredSize];
      CACHE.set(next);
      return next;
    }
    return current;
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
  private static native Object hostInvoke(long name, long pyObject,
          long returnType, long[] argsTypes, Object[] args, int len);

}
