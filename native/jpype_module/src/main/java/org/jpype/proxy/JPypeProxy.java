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
package org.jpype.proxy;

import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodHandles.Lookup;
import java.lang.invoke.MethodType;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.jpype.JPypeContext;
import org.jpype.manager.TypeManager;
import org.jpype.ref.JPypeReferenceQueue;

/**
 * A proxy implementation for JPype that bridges Java and Python.
 *
 * <p>
 * This class acts as an {@link InvocationHandler} for dynamically created proxy
 * objects. It allows Java methods to be invoked on Python objects and handles
 * default method invocation for Java interfaces. The proxy is tied to a
 * specific JPype context and manages cleanup operations for Python objects.</p>
 *
 * <p>
 * Key features include:</p>
 * <ul>
 * <li>Dynamic proxy creation for Java interfaces</li>
 * <li>Reflection-based invocation of default methods</li>
 * <li>Efficient type resolution for method parameters and return types</li>
 * </ul>
 *
 * <p>
 * Note: This class relies on Java reflection and native methods for its
 * functionality.</p>
 */
public class JPypeProxy implements InvocationHandler
{

  // Constructor for accessing default methods in interfaces (Java 8+)
  private final static Constructor<Lookup> constructor;

  // Reference queue for managing cleanup of Python objects
  private final static JPypeReferenceQueue referenceQueue = JPypeReferenceQueue.getInstance();

  // JPype context associated with this proxy
  JPypeContext context;

  // Instance ID of the Python object being proxied
  public long instance;

  // Cleanup function ID for the Python object
  public long cleanup;

  // Interfaces implemented by the proxy
  Class<?>[] interfaces;

  // ClassLoader used for proxy creation
  ClassLoader cl = ClassLoader.getSystemClassLoader();

  // Special object used to indicate a missing implementation
  public static Object missing = new Object();

  // Static block to initialize the constructor for default method invocation
  static
  {
    Constructor<Lookup> c = null;
    if (System.getProperty("java.version").startsWith("1."))
    { // Check for Java 8
      try
      {
        c = Lookup.class.getDeclaredConstructor(Class.class);
        c.setAccessible(true); // Make the constructor accessible
      } catch (NoSuchMethodException | SecurityException ex)
      {
        Logger.getLogger(JPypeProxy.class.getName()).log(Level.SEVERE, null, ex);
      }
    }
    constructor = c;
  }

  /**
   * Creates a new proxy instance.
   *
   * @param context The JPype context associated with the proxy.
   * @param instance The instance ID of the Python object being proxied.
   * @param cleanup The cleanup function ID for the Python object.
   * @param interfaces The Java interfaces to be implemented by the proxy.
   * @return A new {@link JPypeProxy} instance.
   */
  public static JPypeProxy newProxy(JPypeContext context, long instance, long cleanup, Class<?>[] interfaces)
  {
    JPypeProxy proxy = new JPypeProxy();
    proxy.context = context;
    proxy.instance = instance;
    proxy.interfaces = interfaces;
    proxy.cleanup = cleanup;

    // Determine the appropriate class loader for the proxy
    for (Class<?> cls : interfaces)
    {
      ClassLoader icl = cls.getClassLoader();
      if (icl != null && icl != proxy.cl)
        proxy.cl = icl; // Use the custom class loader if necessary
    }
    return proxy;
  }

  /**
   * Creates a new proxy instance that implements the specified interfaces.
   *
   * <p>
   * The proxy is registered with the reference queue for cleanup.</p>
   *
   * @return The dynamically created proxy object.
   */
  public Object newInstance()
  {
    Object out = Proxy.newProxyInstance(cl, interfaces, this);
    referenceQueue.registerRef(out, instance, cleanup); // Register for cleanup
    return out;
  }

  /**
   * Handles method invocation on the proxy object.
   *
   * @param proxy The proxy instance on which the method is invoked.
   * @param method The method being invoked.
   * @param args The arguments passed to the method.
   * @return The result of the method invocation.
   * @throws Throwable If an error occurs during method invocation.
   */
  @Override
  public Object invoke(Object proxy, Method method, Object[] args) throws Throwable
  {
    // Check if the JPype context is shutting down
    if (context.isShutdown())
      throw new RuntimeException("Proxy called during shutdown");

    // Resolve method parameter and return types
    TypeManager typeManager = context.getTypeManager();
    long returnType;
    long[] parameterTypes;
    synchronized (typeManager)
    { // Synchronize type resolution
      returnType = typeManager.findClass(method.getReturnType());
      Class<?>[] types = method.getParameterTypes();
      parameterTypes = new long[types.length];
      for (int i = 0; i < types.length; ++i)
      {
        parameterTypes[i] = typeManager.findClass(types[i]);
      }
    }

    // Attempt to invoke the method on the Python object
    Object result = hostInvoke(context.getContext(), method.getName(), instance, returnType, parameterTypes, args, missing);

    // Return the result if the method is implemented in Python
    if (result != missing)
      return result;

    // Handle default methods in interfaces
    if (method.isDefault())
    {
      try
      {
        Class<?> cls = method.getDeclaringClass();

        // Use the appropriate reflection mechanism based on Java version
        if (constructor != null)
        { // Java 8
          return constructor.newInstance(cls)
                  .findSpecial(cls, method.getName(), MethodType.methodType(method.getReturnType()), cls)
                  .bindTo(proxy)
                  .invokeWithArguments(args);
        }

        // Java 9+
        return MethodHandles.lookup()
                .findSpecial(cls, method.getName(), MethodType.methodType(method.getReturnType()), cls)
                .bindTo(proxy)
                .invokeWithArguments(args);
      } catch (java.lang.IllegalAccessException ex)
      {
        throw new RuntimeException(ex);
      }
    }

    // Throw an exception if no implementation is found
    throw new NoSuchMethodError(method.getName());
  }

  /**
   * Native method to invoke a method on the Python object.
   *
   * @param context is the JPype context.
   * @param name is the name of the method to invoke.
   * @param pyObject is the instance ID of the Python object.
   * @param returnType is the return type of the method.
   * @param argsTypes is the types of the method parameters.
   * @param args is the arguments passed to the method.
   * @param bad is the object indicating a missing implementation.
   * @return the result of the method invocation.
   */
  private static native Object hostInvoke(long context, String name, long pyObject,
          long returnType, long[] argsTypes, Object[] args, Object bad);
}
