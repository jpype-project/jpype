// --- file: org/jpype/proxy/JPypeProxyType.java ---
package org.jpype.proxy;

import java.lang.invoke.MethodHandle;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import org.jpype.JPypeContext;
import org.jpype.manager.TypeManager;
import org.jpype.ref.JPypeReferenceQueue;
import org.jpype.annotation.Bypass;

public final class JPypeProxyType
{

  // Static cache for standard Object methods to avoid re-resolving them
  private static final Map<Method, JPypeMethodDescriptor> OBJECT_METHOD_CACHE = new HashMap<>();
  private final Class<?>[] interfaces;
  private final ClassLoader cl;
  final long cleanup;
  final Map<Method, JPypeMethodDescriptor> methodCache;
  
  /**
   * Initializes the static cache for Object methods. This happens once when the
   * class is loaded.
   *
   * @param tm
   */
  public static void init(TypeManager tm)
  {
    synchronized (tm)
    {
      for (Method method : Object.class.getMethods())
      {
        long returnType = tm.findClass(method.getReturnType());
        Class<?>[] params = method.getParameterTypes();
        long[] paramTypes = new long[params.length];
        for (int i = 0; i < params.length; i++)
          paramTypes[i] = tm.findClass(params[i]);
        OBJECT_METHOD_CACHE.put(method, new JPypeMethodDescriptor(method.getName(), returnType, paramTypes, null, false));
      }
    }
  }

  public JPypeProxyType(long cleanup, Class<?>[] interfaces)
  {
    this.interfaces = interfaces;
    this.cleanup = cleanup;

    // Pin the loader to the org.jpype module loader as the baseline default
    ClassLoader tempCl = JPypeProxyType.class.getClassLoader();
    if (tempCl == null)
      tempCl = ClassLoader.getSystemClassLoader();

    // Only override if an interface explicitly comes from an independent classloader
    for (Class<?> cls : interfaces)
    {
      ClassLoader icl = cls.getClassLoader();
      // Ignore the bootstrap loader (null) and our own loader
      if (icl != null && icl != tempCl && icl != ClassLoader.getSystemClassLoader())
      {
        tempCl = icl;
        break; // Stop if we hit an explicit custom application/plugin loader
      }
    }
    this.cl = tempCl;

    // Build the instance-specific cache
    Map<Method, JPypeMethodDescriptor> tempMap = new HashMap<>();

    // 1. Bulk copy the pre-resolved Object methods
    tempMap.putAll(OBJECT_METHOD_CACHE);

    // 2. Resolve interface-specific methods
    TypeManager tm = JPypeContext.getInstance().getTypeManager();
    synchronized (tm)
    {
      for (Class<?> iface : interfaces)
        populateCache(tm, iface.getMethods(), tempMap);
    }

    this.methodCache = Collections.unmodifiableMap(tempMap);
  }

  private void populateCache(TypeManager tm, Method[] methods, Map<Method, JPypeMethodDescriptor> map)
  {
    for (Method method : methods)
    {
      if (map.containsKey(method))
        continue;

      long returnType = tm.findClass(method.getReturnType());
      Class<?>[] params = method.getParameterTypes();
      long[] paramTypes = new long[params.length];
      for (int i = 0; i < params.length; i++)
        paramTypes[i] = tm.findClass(params[i]);

      boolean bypass = (method.isAnnotationPresent(Bypass.class));
      MethodHandle defaultHandle = null;
      if (method.isDefault())
        defaultHandle = getDefaultHandle(method.getDeclaringClass(), method, java.lang.invoke.MethodHandles.class);
      map.put(method, new JPypeMethodDescriptor(method.getName(), returnType, paramTypes, defaultHandle, bypass));
    }
  }

  public JPypeMethodDescriptor getMethodDescriptor(Method method)
  {
    return methodCache.get(method);
  }

  public Object newInstance(long instance)
  {
    JPypeProxyInstance handler = new JPypeProxyInstance(this, instance);
    Object proxy = Proxy.newProxyInstance(cl, interfaces, handler);
    JPypeReferenceQueue.getInstance().registerRef(proxy, instance, cleanup);
    return proxy;
  }

  native MethodHandle getDefaultHandle(Class<?> cls, Method method, Class<?> mhCls);

  @SuppressWarnings("unused")
  private static long unwrapPythonException(Throwable throwable)
  {
    if (throwable == null)
      return 0;
    if (throwable instanceof python.exceptions.PyBaseException)
      return unwrapObject(((python.exceptions.PyBaseException) throwable).get());
    if (throwable instanceof python.lang.PyExc)
      return unwrapObject(throwable);
    return 0;
  }

  private static long unwrapObject(Object obj)
  {
    if (!Proxy.isProxyClass(obj.getClass()))
      return 0;

    try
    {
      InvocationHandler handler = Proxy.getInvocationHandler(obj);
      if (handler instanceof JPypeProxyInstance)
      {
        JPypeProxyInstance jpypeHandler = (JPypeProxyInstance) handler;
        return jpypeHandler.instance;
      }
    } catch (IllegalArgumentException e)
    {
    }
    return 0;
  }

  // exports to JNI
  @SuppressWarnings("unused")
  private static long getInstance(Object obj)
  {
    if (obj == null || !Proxy.isProxyClass(obj.getClass()))
      return 0L;
    InvocationHandler handler = Proxy.getInvocationHandler(obj);
    if (!(handler instanceof JPypeProxyInstance))
      return 0L;
    JPypeProxyInstance proxy = ((JPypeProxyInstance) handler);
    return proxy.instance;
  }

}
