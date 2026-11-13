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
package jpype.classhints;

public class ClassHintsTest
{

  public static Custom input = null;

  public static void call(Custom c)
  {
    input = c;
  }

  // Overloads for checking that a Custom whose initialization failed does not
  // break resolution of the other candidate.  Object is never an exact match,
  // so Custom is always tested.
  public static String pick(Object o)
  {
    return "Object";
  }

  public static String pick(Custom c)
  {
    return "Custom";
  }
}
