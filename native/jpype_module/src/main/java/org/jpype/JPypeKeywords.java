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
package org.jpype;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 *
 * @author nelson85
 */
public class JPypeKeywords
{

  final public static Set<String> keywords = new HashSet<>();

  public static void setKeywords(String[] s)
  {
    keywords.addAll(Arrays.asList(s));
  }

  public static String wrap(String name)
  {
    if (keywords.contains(name))
      return name + "_";
    return name;
  }

  public static String unwrap(String name)
  {
    if (!name.endsWith("_"))
      return name;
    String name2 = name.substring(0, name.length() - 1);
    if (keywords.contains(name2))
      return name2;;
    return name;
  }

  static String safepkg(String s)
  {
    if (!s.contains("_"))
      return s;
    String[] parts = s.split("\\.");
    for (int i = 0; i < parts.length; ++i)
      parts[i] = unwrap(parts[i]);
    return String.join(".", parts);
  }
}
