// --- file: org/jpype/proxy/JPypeProxyType.java ---
package org.jpype.proxy;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import org.jpype.JPypeContext;
import org.jpype.manager.TypeManager;
import org.jpype.ref.JPypeReferenceQueue;

public class JPypeProxyType
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
        OBJECT_METHOD_CACHE.put(method, new JPypeMethodDescriptor(method.getName(), returnType, paramTypes, null));
      }
    }
  }

  public JPypeProxyType(long cleanup, Class<?>[] interfaces)
  {
    this.interfaces = interfaces;
    this.cleanup = cleanup;

    // Resolve ClassLoader
    ClassLoader tempCl = ClassLoader.getSystemClassLoader();
    for (Class<?> cls : interfaces)
    {
      ClassLoader icl = cls.getClassLoader();
      if (icl != null && icl != tempCl)
        tempCl = icl;
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

      MethodHandle defaultHandle = null;
      if (method.isDefault())
        defaultHandle = getDefaultHandle(method.getDeclaringClass(), method, java.lang.invoke.MethodHandles.class);
      map.put(method, new JPypeMethodDescriptor(method.getName(), returnType, paramTypes, defaultHandle));
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

}
