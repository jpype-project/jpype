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
package jpype.str;

public class Test
{

  public static String staticField = "staticField";
  public String memberField = "memberField";

  public static String staticCall()
  {
    return "staticCall";
  }

  public String memberCall()
  {
    return "memberCall";
  }

  public static String callWithNullBytes() {
    return "call\0With\0Null\0Bytes";
  }

  public static String returnArgument(String argument) {
    return argument;
  }

  public static final String array[] =
  {
    "apples", "banana", "cherries", "dates", "elderberry"
  };

  public static String callProxy(StringFunction f, String s)
  {
    return f.call(s);
  }
}
