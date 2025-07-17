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
 * Utility class for handling reserved keywords in JPype.
 *
 * This class provides methods to manage a set of reserved keywords and offers
 * functionality to wrap and unwrap names to avoid conflicts with these
 * keywords. It also includes a method to safely process package names
 * containing underscores.
 */
public class JPypeKeywords
{

  /**
   * A set of reserved keywords.
   *
   * This set contains keywords that need special handling to avoid conflicts.
   * Keywords can be added dynamically using the {@link #setKeywords(String[])}
   * method.
   */
  final public static Set<String> keywords = new HashSet<>();

  /**
   * Adds keywords to the reserved keywords set.
   *
   * This method allows dynamic addition of keywords to the {@link #keywords}
   * set.
   *
   * @param s An array of strings representing the keywords to add.
   */
  public static void setKeywords(String[] s)
  {
    keywords.addAll(Arrays.asList(s));
  }

  /**
   * Wraps a name if it matches a reserved keyword.
   *
   * If the given name is a reserved keyword, this method appends an underscore
   * (`_`) to the name to avoid conflicts. Otherwise, it returns the name
   * unchanged.
   *
   * @param name The name to check and wrap if necessary.
   * @return The wrapped name if it is a keyword, otherwise the original name.
   */
  public static String wrap(String name)
  {
    if (keywords.contains(name))
    {
      return name + "_";
    }
    return name;
  }

  /**
   * Unwraps a name that ends with an underscore (`_`).
   *
   * If the given name ends with an underscore and matches a reserved keyword
   * (after removing the underscore), this method returns the original keyword.
   * Otherwise, it returns the name unchanged.
   *
   * @param name The name to check and unwrap if necessary.
   * @return The unwrapped name if it ends with an underscore and matches a
   * keyword, otherwise the original name.
   */
  public static String unwrap(String name)
  {
    if (!name.endsWith("_"))
    {
      return name;
    }
    String name2 = name.substring(0, name.length() - 1);
    if (keywords.contains(name2))
    {
      return name2;
    }
    return name;
  }

  /**
   * Safely processes a package name by unwrapping parts containing underscores.
   *
   * This method splits the package name into its components (separated by
   * dots), unwraps each part, and then reassembles the package name. It ensures
   * that underscores are handled properly for each part of the package name.
   *
   * @param s The package name to process.
   * @return The processed package name with unwrapped parts.
   */
  static String safepkg(String s)
  {
    if (!s.contains("_"))
    {
      return s;
    }
    String[] parts = s.split("\\.");
    for (int i = 0; i < parts.length; ++i)
    {
      parts[i] = unwrap(parts[i]);
    }
    return String.join(".", parts);
  }
}
