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

// Scratch harness for benchmarking the Java->Python return path.
// Not wired into any test yet -- used from an ad hoc benchmark script.
public class Bench
{

  public static Object identity(Object o)
  {
    return o;
  }

  public static int identityInt(int x)
  {
    return x;
  }

  public static void sink(Object o)
  {
  }
}
