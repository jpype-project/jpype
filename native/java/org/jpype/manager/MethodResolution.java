/*
 *    Copyright 2019 Karl Einar Nelson
 *   
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *  
 *     http://www.apache.org/licenses/LICENSE-2.0
 *  
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
package org.jpype.manager;

import java.lang.reflect.Executable;
import java.util.ArrayList;
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

        if (isMoreSpecificThan(m1.executable, m2.executable) && !isMoreSpecificThan(m2.executable, m1.executable))
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
    Class<?>[] param1 = method1.getParameterTypes();
    Class<?>[] param2 = method2.getParameterTypes();

    if (param1.length != param2.length)
      return false;

    for (int i = 0; i < param1.length; ++i)
    {
      if (!param1[i].isAssignableFrom(param2[i]))
        return false;
    }
    return true;
  }
}
