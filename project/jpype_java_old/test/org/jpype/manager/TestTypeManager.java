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

import org.jpype.manager.TypeFactoryHarness.DeletedResource;
import org.jpype.manager.TypeFactoryHarness.Resource;

/**
 *
 * @author nelson85
 */
public class TestTypeManager
{

  public interface MyFunction
  {

    Object apply(Object o);
  }

  public static class MyBase
  {

    void run()
    {
    }
  }

  static public void main(String[] args)
  {
    System.out.println("Create:");
    TypeManager tm = new TypeManager();
    TypeFactoryHarness tf = new TypeFactoryHarness(tm);
    tm.typeFactory = tf;
    System.out.println("Initialize:");
    tm.init();
    System.out.println();
    System.out.println("==================================================");
    tm.findClass(int[][][].class);
    MyFunction f = (Object o) -> o;
    tm.findClass(f.getClass());

    MyFunction f2 = new MyFunction()
    {
      @Override
      public Object apply(Object t)
      {
        return t;
      }
    };
    tm.findClass(f2.getClass());

    MyBase f3 = new MyBase()
    {
      void run()
      {
      }
    };
    tm.findClass(f3.getClass());

    System.out.println("==================================================");
    System.out.println();
    System.out.println("Shutdown:");
    tm.shutdown();

    System.out.println("Leaked resources:");
    int leaked = 0;
    for (Resource entry : tf.resourceMap.values())
    {
      if (entry instanceof DeletedResource)
        continue;
      System.out.println("  " + entry.getName());
      leaked++;
    }
    System.out.println("Resource created " + tf.value); // was 8803, reduced to 1149
    System.out.println("Leaked total " + leaked);
    if (leaked > 0)
      throw new RuntimeException("Leaked resources");
  }
}
