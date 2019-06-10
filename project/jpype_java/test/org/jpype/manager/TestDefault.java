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

import java.lang.reflect.Method;
import java.util.EnumSet;

/**
 *
 * @author nelson85
 */
public class TestDefault
{
  public interface Foo
  {
    default void fred()
    {
    }

    void george();

    static void bob()
    {
    }
  }

  static public void main(String[] args)
  {
    System.out.println("declared:");
    for (Method method : int[].class.getDeclaredMethods())
      System.out.println("  " + method);

    System.out.println("methods:");
    for (Method method : int[].class.getMethods())
    {
      System.out.println("  " + method);
    }

    System.out.println(int[][][].class.getCanonicalName());
    System.out.println(int[][][].class.getName());
    System.out.println(int[][][].class.getSimpleName());

    System.out.println(Object[][][].class.getCanonicalName());
    System.out.println(Object[][][].class.getName());
    System.out.println(Object[][][].class.getSimpleName());

    System.out.println(Object.class.getSimpleName());
    System.out.println(Object.class.getName());
    System.out.println(Object.class.getCanonicalName());

    EnumSet<ModifierCode> set = EnumSet.of(ModifierCode.PUBLIC, ModifierCode.CTOR);
    System.out.println(set);
  }
}
