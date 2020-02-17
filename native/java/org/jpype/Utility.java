/*
 * Copyright 2018, Karl Nelson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
package org.jpype;

import java.lang.annotation.Annotation;
import java.lang.reflect.Array;
import java.lang.reflect.Proxy;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.jpype.proxy.JPypeInvocationHandler;

/**
 * Support for JPype TypeManager.
 *
 * This is an internal class containing backported functionality from JPype 0.8.
 * All functionality in this class will be moved in JPype 0.8 when the type
 * manager functionality moved into the Java layer.
 *
 */
public class Utility
{

  static boolean hasCallerSensitive = false;

  static
  {
    try
    {
      java.lang.reflect.Method method = java.lang.Class.class.getDeclaredMethod("forName", String.class);
      for (Annotation annotation : method.getAnnotations())
      {
        if ("@jdk.internal.reflect.CallerSensitive()".equals(annotation.toString()))
        {
          hasCallerSensitive = true;
        }
      }
    } catch (NoSuchMethodException | SecurityException ex)
    {
    }
  }

  /**
   * Checks to see if the method is caller sensitive.
   *
   * As the annotation is a private internal, we must check by name.
   *
   * @param method is the method to be probed.
   * @return true if caller sensitive.
   */
  public static boolean isCallerSensitive(Method method)
  {
    if (hasCallerSensitive)
    {
      for (Annotation annotation : method.getAnnotations())
      {
        if ("@jdk.internal.reflect.CallerSensitive()".equals(annotation.toString()))
        {
          return true;
        }
      }
    } else
    {
      // JDK prior versions prior to 9 do not annotate methods that
      // require special handling, thus we will just blanket those
      // classes known to have issues.
      Class<?> cls = method.getDeclaringClass();
      if (cls.equals(java.lang.Class.class)
              || cls.equals(java.lang.ClassLoader.class)
              || cls.equals(java.sql.DriverManager.class))
      {
        return true;
      }
    }
    return false;
  }

  /**
   * Call a method using reflection.
   *
   * This method creates a stackframe so that caller sensitive methods will
   * execute properly.
   *
   * @param method is the method to call.
   * @param obj is the object to operate on, it will be null if the method is
   * static.
   * @param args the arguments to method.
   * @return the object that results form the invocation.
   * @throws IllegalAccessException
   * @throws IllegalArgumentException
   * @throws InvocationTargetException
   */
  public static Object callMethod(Method method, Object obj, Object[] args)
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
   * Get the class for an object.
   * <p>
   * This method is somewhat misplaced, but we don't have a good place for it in
   * JPype 0.7. This prevents generation of pointless wrappers for lambda and
   * other synthetic classes. This functionality moves to TypeManager in 0.8.
   *
   * @param obj is the object to probe.
   * @return the class to use for JPype
   */
  public static Class<?> getClassFor(Object obj)
  {
    Class cls = obj.getClass();
    if (Proxy.isProxyClass(cls) && (Proxy.getInvocationHandler(obj) instanceof org.jpype.proxy.JPypeInvocationHandler))
    {
      return JPypeInvocationHandler.class;
    }

    if (cls.isSynthetic())
    {
      // We may be a lambda or a Proxy.

      // We are a lambda (Lambda can only have one interface.
      return cls.getInterfaces()[0];
    }
    return cls;
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
  public static Object[] collectRectangular(Object o)
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
      shape[d++] = Array.getLength(o1);
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

}
