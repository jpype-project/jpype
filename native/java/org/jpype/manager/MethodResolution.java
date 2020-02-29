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

    // This line prevents ambiguity resolution both static and method forms.
//    if (Modifier.isStatic(method1.getModifiers())
//            != Modifier.isStatic(method2.getModifiers()))
//      return false;

    if (!Modifier.isStatic(method1.getModifiers()))
      param1.add(0, method1.getDeclaringClass());
    if (!Modifier.isStatic(method2.getModifiers()))
      param2.add(0, method2.getDeclaringClass());

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
