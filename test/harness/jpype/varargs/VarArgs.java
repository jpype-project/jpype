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
package jpype.varargs;

import java.util.Map;

class VarArgs
{

  public Object[] rest;

  public static Object[] call(Object... args)
  {
    return args;
  }

  public static String[] callString(String... args)
  {
    return args;
  }

  public static int callString0(String str, String... args)
  {
    return args.length;
  }

  public int callString1(String str, String... args)
  {
    return args.length;
  }

  public static Integer callOverload(Integer i)
  {
    return i;
  }

  public static String[] callOverload(String str, String... rest)
  {
    return rest;
  }

  public VarArgs()
  {
  }

  public VarArgs(String s, Object... rest)
  {
    this.rest = rest;
  }

  public String[] method(String s, String... rest)
  {
    return rest;
  }

  public int conflict1(Object... j)
  {
  	return 1;
  }

  public int conflict1(Map j)
  {
  	return 2;
  }

  public int conflict2(Object... j)
  {
  	return 1;
  }

  public int conflict2(Map j, Map k)
  {
  	return 2;
  }


  public int conflict3(char... j)
  {
  	return 1;
  }

  public int conflict3(String j)
  {
  	return 2;
  }

  // Conflict 4 - overloaded multi-param signature, vararg of identical type

  public int conflict4(Object o, double... j)
  {
  	return 1;
  }

  public int conflict4(Object o, double j)
  {
  	return 2;
  }

  // Conflict 5 - overloaded single-param signature, vararg of identical type
  // NB: pre-patch, conflict4 deterministically generated an ambiguous match error.
  //     However, conflict5 would arbitrarily pick between these two signatures when
  //     called with a single variable, resulting in unpredictable failures.

  public int conflict5(double... j)
  {
  	return 1;
  }

  public int conflict5(double j)
  {
  	return 2;
  }
}
