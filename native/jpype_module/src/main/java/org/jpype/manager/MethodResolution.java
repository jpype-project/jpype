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
package org.jpype.manager;

import java.lang.reflect.Executable;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;

/**
 * Sort out which methods hide other methods.
 * <p>
 * When resolving method overloads there may be times in which more than one
 * overload applies. JPype requires that the methods appear in order from most
 * to least specific. And each method overload requires a list of methods that
 * are more general. If two or more methods match and one is not more specific
 * than the other.
 *
 * @author nelson85
 */
public class MethodResolution
{

  long ptr = 0;
  boolean covered = false;
  Executable executable;
  List<MethodResolution> children = new ArrayList<>();

  MethodResolution(Executable method)
  {
    this.executable = method;
  }

  private boolean isCovered()
  {
    for (MethodResolution ov : this.children)
    {
      if (!ov.covered)
        return false;
    }
    covered = true;
    return true;
  }

  /**
   * Order methods from least to most specific.
   *
   * @param <T>
   * @param methods
   * @return
   */
  public static <T extends Executable>
          List<MethodResolution> sortMethods(List<T> methods)
  {
    // Create a method resolution for each method
    LinkedList<MethodResolution> unsorted = new LinkedList<>();
    for (T m1 : methods)
    {
      unsorted.add(new MethodResolution(m1));
    }

    for (MethodResolution m1 : unsorted)
    {
      for (MethodResolution m2 : unsorted)
      {
        if (m1 == m2)
          continue;

        if (isMoreSpecificThan(m1.executable, m2.executable)
                && !isMoreSpecificThan(m2.executable, m1.executable))
        {
          m1.children.add(m2);
        }
      }
    }

    // Execute a graph sort problem so that the most specific are always on the front
    LinkedList<MethodResolution> out = new LinkedList<>();
    while (!unsorted.isEmpty())
    {
      // Remove the first unsorted element
      MethodResolution front = unsorted.pop();
      // Check to see if all dependencies are already ordered
      boolean good = front.isCovered();

      // If all dependencies are included
      if (good)
      {
        front.covered = true;
        out.add(front);
      } else
      {
        unsorted.add(front);
      }
    }
    return out;
  }

  // Table for primitive rules
  static Class[] of(Class... l)
  {
    return l;
  }
  static HashMap<Class, Class[]> CONVERSION = new HashMap<>();

  
  {
    CONVERSION.put(Byte.TYPE,
            of(Byte.TYPE, Byte.class, Short.TYPE, Short.class,
                    Integer.TYPE, Integer.class, Long.TYPE, Long.class,
                    Float.TYPE, Float.class, Double.TYPE, Double.class));
    CONVERSION.put(Character.TYPE,
            of(Character.TYPE, Character.class, Short.TYPE, Short.class,
                    Integer.TYPE, Integer.class, Long.TYPE, Long.class,
                    Float.TYPE, Float.class, Double.TYPE, Double.class));
    CONVERSION.put(Short.TYPE,
            of(Short.TYPE, Short.class,
                    Integer.TYPE, Integer.class, Long.TYPE, Long.class,
                    Float.TYPE, Float.class, Double.TYPE, Double.class));
    CONVERSION.put(Integer.TYPE,
            of(Integer.TYPE, Integer.class, Long.TYPE, Long.class,
                    Float.TYPE, Float.class, Double.TYPE, Double.class));
    CONVERSION.put(Long.TYPE,
            of(Long.TYPE, Long.class,
                    Float.TYPE, Float.class, Double.TYPE, Double.class));
    CONVERSION.put(Float.TYPE,
            of(Float.TYPE, Float.class, Double.TYPE, Double.class));
    CONVERSION.put(Double.TYPE,
            of(Double.TYPE, Double.class));
    CONVERSION.put(Boolean.TYPE,
            of(Boolean.TYPE, Boolean.class));
  }

  static boolean isAssignableTo(Class c1, Class c2)
  {
    if (!c1.isPrimitive())
      return c2.isAssignableFrom(c1);
    Class[] cl = CONVERSION.get(c1);
    if (cl == null)
      return false;
    for (Class c3 : cl)
      if (c2.equals(c3))
        return true;
    return false;
  }

  /**
   * Determine is a executable is more specific than another.
   * <p>
   * This is public so that we can debug from within jpype.
   *
   * @param method1
   * @param method2
   * @return
   */
  public static boolean isMoreSpecificThan(Executable method1, Executable method2)
  {
    List<Class<?>> param1 = new ArrayList<>(Arrays.asList(method1.getParameterTypes()));
    List<Class<?>> param2 = new ArrayList<>(Arrays.asList(method2.getParameterTypes()));

    if (!Modifier.isStatic(method1.getModifiers()))
      param1.add(0, method1.getDeclaringClass());
    if (!Modifier.isStatic(method2.getModifiers()))
      param2.add(0, method2.getDeclaringClass());

    // Special handling is needed for varargs as it may chop or expand.
    // we have 4 cases for a varargs methods
    //    foo(Arg0, Arg1...) as
    //       foo(Arg0)
    //       foo(Arg0, Arg1)
    //       foo(Arg0, Arg1[])
    //       foo(Arg0, Arg1, Arg1+)
    if (method1.isVarArgs() && method2.isVarArgs())
    {
      // Punt on this as there are too many different cases
      return isMoreSpecificThan(param1, param2);
    }

    if (method1.isVarArgs())
    {
      int n1 = param1.size();
      int n2 = param2.size();

      // Last element is an array
      Class<?> cls = param1.get(n1 - 1);
      Class<?> cls2 = cls.getComponentType();

      // Less arguments, chop the list 
      if (n1 - 1 == n2)
        return isMoreSpecificThan(param1.subList(0, n2), param2);

      // Same arguments
      if (n1 == n2)
      {
        List<Class<?>> q = new ArrayList<>(param1);
        q.set(n1 - 1, cls2);

        // Check both ways
        boolean isMoreSpecific = isMoreSpecificThan(param1, param2) || isMoreSpecificThan(q, param2);

        // If the varargs array is of the single-variable's type (or they are primitive-equivalent),
        // the single-variable signature should win specificity
        Class<?> svCls = param2.get(n2 - 1);
        return isMoreSpecific && !(isAssignableTo(cls2, svCls) && isAssignableTo(svCls, cls2));
      }

      // More arguments
      if (n1 < n2)
      {
        // Grow the list
        List<Class<?>> q = new ArrayList<>(param1);
        q.set(n1 - 1, cls2);
        for (int i = n1; i < n2; ++i)
          q.add(cls2);
        return isMoreSpecificThan(q, param2);
      }
    }

    if (method2.isVarArgs())
    {
      int n1 = param1.size();
      int n2 = param2.size();

      // Last element is an array
      Class<?> cls = param2.get(n2 - 1);
      Class<?> cls2 = cls.getComponentType();

      // Less arguments, chop the list
      if (n2 - 1 == n1)
        return isMoreSpecificThan(param1, param2.subList(0, n2));

      // Same arguments
      if (n1 == n2)
      {
        List<Class<?>> q = new ArrayList<>(param2);
        q.set(n2 - 1, cls2);

        // Compare both ways
        return isMoreSpecificThan(param1, param2) || isMoreSpecificThan(param1, q);
      }

      // More arguments
      if (n2 < n1)
      {
        // Grow the list
        List<Class<?>> q = new ArrayList<>(param2);
        q.set(n2 - 1, cls2);
        for (int i = n2; i < n1; ++i)
          q.add(cls2);
        return isMoreSpecificThan(param1, q);
      }
    }

    return isMoreSpecificThan(param1, param2);
  }

  public static boolean isMoreSpecificThan(List<Class<?>> param1, List<Class<?>> param2)
  {
    // FIXME need to consider resolving mixing of static and non-static
    // Methods here.
    if (param1.size() != param2.size())
      return false;

    for (int i = 0; i < param1.size(); ++i)
    {
      if (!isAssignableTo(param1.get(i), param2.get(i)))
        return false;
    }
    return true;
  }
}
