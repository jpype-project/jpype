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
package org.jpype;

import java.lang.reflect.Array;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.Buffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.jpype.manager.TypeFactory;
import org.jpype.manager.TypeFactoryNative;
import org.jpype.manager.TypeManager;
import org.jpype.ref.JPypeReferenceQueue;

/**
 * Context for JPype.
 * <p>
 * This is the part of JPype that holds all resources. After the classloader is
 * created this class is given the address of the context object in JPype. Any
 * resources in JPype Java layer can be contacted using the context.
 * <p>
 * Boot order is - create the C++ portion of the context. - start the jvm - load
 * the bootloader - install the jar into the bootloader - install all native
 * methods using the bootloader - create the Java portion of the context. - use
 * the Java context to access the resources (ReferenceQueue, TypeFactory,
 * TypeManager)
 * <p>
 * Once started, python calls use the context to get a frame and attach their
 * threads. Methods called from Java will get the env and use it to get their
 * context from which they can create a frame.
 * <p>
 * The C++ context will hold all the previous global variables thus allowing the
 * C++ portion to be cleaned up properly when the JVM is shutdown or
 * disconnected.
 * <p>
 * As the JPypeContext can't be tested directly from Java code, it will need to
 * be kept light.
 * <p>
 * Our goal is to remove as much direct contact methods as possible from the C++
 * layer. Previous globals in JPTypeManager move to the context as do the
 * contents of JPJni.
 *
 *
 *
 * @author nelson85
 */
public class JPypeContext
{

  private static JPypeContext instance = null;
  // This is the C++ portion of the context.
  private long context;
  private TypeFactory typeFactory;
  private TypeManager typeManager;
  private JPypeReferenceQueue referenceQueue;
  private ClassLoader bootLoader;

  static public JPypeContext getInstance()
  {
    return instance;
  }

  /**
   * Start the JPype system.
   *
   * @param context is the C++ portion of the context.
   * @param bootLoader is the classloader holding JPype resources.
   * @return the created context.
   */
  public static JPypeContext createContext(long context, ClassLoader bootLoader)
  {
    instance = new JPypeContext();

    instance.context = context;
    instance.bootLoader = bootLoader;
    instance.typeFactory = new TypeFactoryNative();
    instance.typeManager = new TypeManager(context, instance.typeFactory);
    instance.typeManager.typeFactory = instance.typeFactory;

    instance.referenceQueue = new JPypeReferenceQueue(context);

    // Okay everything is setup so lets give it a go.
    instance.typeManager.init();
    instance.referenceQueue.start();

    return instance;
  }

  /**
   * Stop all JPype resources.
   *
   */
  public void shutdown()
  {
    this.referenceQueue.stop();
    this.typeManager.shutdown();
  }

  /**
   * Get the C++ portion.
   *
   * @return
   */
  public long getContext()
  {
    return context;
  }

  public ClassLoader getBootLoader()
  {
    return this.bootLoader;
  }

  public TypeFactory getTypeFactory()
  {
    return this.typeFactory;
  }

  public TypeManager getTypeManager()
  {
    return this.typeManager;
  }

  public JPypeReferenceQueue getReferenceQueue()
  {
    return this.referenceQueue;
  }

  /**
   * Call a method using reflection.This method creates a stackframe so that
   * caller sensitive methods will execute properly.
   *
   *
   * @param method is the method to call.
   * @param obj is the object to operate on, it will be null if the method is
   * static.
   * @param args the arguments to method.
   * @return the object that results form the invocation.
   * @throws java.lang.Throwable throws whatever type the called method
   * produces.
   */
  public Object callMethod(Method method, Object obj, Object[] args)
          throws Throwable
  {
    try
    {
      return method.invoke(obj, args);
    } catch (InvocationTargetException ex)
    {
      throw ex.getCause();
    }
  }

  /**
   * Helper function for collect rectangular,
   */
  private static boolean collect(List l, Object o, int q, int[] shape, int d)
  {
    if (Array.getLength(o) != shape[q])
      return false;
    if (q + 1 == d)
    {
      l.add(o);
      return true;
    }
    for (int i = 0; i < shape[q]; ++i)
    {
      if (!collect(l, Array.get(o, i), q + 1, shape, d))
        return false;
    }
    return true;
  }

  /**
   * Collect up a rectangular primitive array for a Python memory view.
   *
   * If it is a rectangular primitive array then the result will be an object
   * array containing. - the primitive type - an int array with the shape of the
   * array - each of the primitive arrays that will need be visited in order.
   *
   * This is the safest way to provide a view as we are verifying and collected
   * thus even if something mutates the shape of the array after we have
   * visited, we have a locked copy.
   *
   * @param o is the object to be tested.
   * @return null if the object is not a rectangular primitive array.
   */
  public Object[] collectRectangular(Object o)
  {
    if (o == null || !o.getClass().isArray())
      return null;
    int[] shape = new int[5];
    int d = 0;
    ArrayList<Object> out = new ArrayList<>();
    Object o1 = o;
    Class c1 = o1.getClass();
    for (int i = 0; i < 5; ++i)
    {
      int l = Array.getLength(o1);
      if (l == 0)
        return null;
      shape[d++] = l;
      o1 = Array.get(o1, 0);
      if (o1 == null)
        return null;
      c1 = c1.getComponentType();
      if (!c1.isArray())
        break;
    }
    if (!c1.isPrimitive())
      return null;
    out.add(c1);
    shape = Arrays.copyOfRange(shape, 0, d);
    out.add(shape);
    int total = 1;
    for (int i = 0; i < d - 1; i++)
      total *= shape[i];
    out.ensureCapacity(total + 2);
    if (d == 5)
      return null;
    if (!collect(out, o, 0, shape, d))
      return null;
    return out.toArray();
  }

  private Object unpack(int size, Object parts)
  {
    Object e0 = Array.get(parts, 0);
    Class c = e0.getClass();
    int segments = Array.getLength(parts) / size;
    Object a2 = Array.newInstance(c, size);
    Object a1 = Array.newInstance(a2.getClass(), segments);
    int k = 0;
    for (int i = 0; i < segments; i++)
    {
      for (int j = 0; j < size; j++, k++)
      {
        Object o = Array.get(parts, k);
        Array.set(a2, j, o);
      }
      Array.set(a1, i, a2);
      if (i < segments - 1)
        a2 = Array.newInstance(c, size);
    }
    return a1;
  }

  public Object assemble(int[] dims, Object parts)
  {
    int n = dims.length;
    if (n == 1)
      return Array.get(parts, 0);
    if (n == 2)
      return Array.get(unpack(dims[0], parts), 0);
    for (int i = 0; i < n - 2; ++i)
    {
      parts = unpack(dims[n - i - 2], parts);
    }
    return parts;
  }

  public static class PyExceptionProxy extends RuntimeException
  {

    long cls, value;

    public PyExceptionProxy(long l0, long l1)
    {
      cls = l0;
      value = l1;
    }
  }

  public long getExcClass(Throwable th)
  {
    if (th instanceof PyExceptionProxy)
      return ((PyExceptionProxy) th).cls;
    return 0;
  }

  public long getExcValue(Throwable th)
  {
    if (th instanceof PyExceptionProxy)
      return ((PyExceptionProxy) th).value;
    return 0;
  }

  public Exception createException(long l0, long l1)
  {
    return new PyExceptionProxy(l0, l1);
  }

  public boolean order(Buffer b)
  {
    if (b instanceof java.nio.ByteBuffer)
      return ((java.nio.ByteBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.ShortBuffer)
      return ((java.nio.ShortBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.CharBuffer)
      return ((java.nio.CharBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.IntBuffer)
      return ((java.nio.IntBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.LongBuffer)
      return ((java.nio.LongBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.FloatBuffer)
      return ((java.nio.FloatBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.DoubleBuffer)
      return ((java.nio.DoubleBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    return true;
  }

}
