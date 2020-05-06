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
