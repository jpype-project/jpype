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

import java.util.Arrays;
import java.util.List;

/**
 *
 * @author Karl Einar Nelson
 */
public class TestMethodResolution
{

  static class Foo
  {

    public static void bar(Object obj, int i)
    {
    }

    public void bar(int i)
    {
    }
  }

  static public void main(String[] args) throws NoSuchMethodException
  {

    List<MethodResolution> foo = MethodResolution
            .sortMethods(Arrays.asList(TestMethodResolution.Foo.class.getDeclaredMethods()));

    for (MethodResolution m : foo)
    {
      System.out.println(m.executable);
      for (MethodResolution m1 : m.children)
      {
        System.out.println("  " + m1.executable);
      }
    }
  }
}
