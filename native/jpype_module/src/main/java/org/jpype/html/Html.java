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
package org.jpype.html;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import org.jpype.JPypeContext;
import org.jpype.pkg.JPypePackageManager;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;

public class Html
{

  public final static HashSet<String> VOID_ELEMENTS = new HashSet<>();
  public final static HashSet<String> OPTIONAL_ELEMENTS = new HashSet<>();
  public final static HashSet<String> OPTIONAL_CLOSE = new HashSet<>();

  static
  {
    VOID_ELEMENTS.addAll(Arrays.asList(
            "area", "base", "br", "col", "command", "embed", "hr", "img",
            "input", "keygen", "link", "meta", "param", "source", "track", "wbr"));
    OPTIONAL_ELEMENTS.addAll(Arrays.asList("html", "head", "body", "p", "dt",
            "dd", "li", "option", "thead", "th", "tbody", "tr", "td", "tfoot", "colgroup"));
    OPTIONAL_CLOSE.addAll(Arrays.asList("li:li", "dt:dd",
            "p:address", "p:article", "p:aside", "p:blockquote", "p:details",
            "p:div", "p:dl", "p:fieldset", "p:figcaption", "p:figure",
            "p:footer", "p:form", "p:h1", "p:h2", "p:h3", "p:h4", "p:h5", "p:h6",
            "p:header", "p:hgroup", "p:hr", "p:main", "p:menu",
            "p:nav", "p:ol", "p:p", "p:pre", "p:section", "p:table", "p:ul",
            "dd:dt", "dd:dd", "dt:dt", "dt:dd", "rt:rt", "rt:rp", "rp:rt", "rp:rp",
            "optgroup:optgroup", "option:option", "option:optiongroup", "thread:tbody",
            "thread:tfoot", "tbody:tfoot", "tbody:tbody", "tr:tr", "td:td", "td:th",
            "th:td", "p:li"));
  }

  public static Parser<Document> newParser()
  {
    return new HtmlParser();
  }

  public static List<Attr> parseAttributes(Document doc, String str)
  {
    AttrParser p = new AttrParser(doc);
    p.parse(str);
    return p.attrs;
  }

//<editor-fold desc="decode" defaultstate="collapsed">
  public static Map<String, Integer> ENTITIES = new HashMap<>();

  static
  {
    // entities.txt ships alongside this class's own .class file (same jar
    // on desktop, same dex on Android), so it's always reachable through
    // this class's own defining classloader. ClassLoader.getSystemClassLoader()
    // works for that by coincidence on desktop (there, the system
    // classloader and this class's own loader are the same app classloader)
    // but not on Android, where getSystemClassLoader() returns a boot-loader
    // stub with no dex visibility at all (see doc/android.rst) - every
    // lookup through it returns null there, which crashed this static
    // initializer with an NPE on Android before this fix.
    // entities.txt is a plain resource file, not a .class - even placed
    // correctly under this class's own defining classloader, it still
    // doesn't reach the built APK on Android (Android Gradle's default
    // java source set silently drops non-.java files from src/main/java;
    // see project/android/recipes/jpype1/__init__.py's
    // generate_package_markers for the same issue with the package-list
    // asset). So on Android this falls back to the bundled copy shipped
    // as a real Android asset (see JPypePackageManager.openAndroidAsset,
    // and buildozer.spec's android.add_assets for how it gets bundled).
    ClassLoader cl = Html.class.getClassLoader();
    InputStream resourceStream = cl.getResourceAsStream("org/jpype/html/entities.txt");
    InputStream is = resourceStream != null ? resourceStream
            : JPypePackageManager.openAndroidAsset("jpype-android-html-entities.txt");
    try (InputStream stream = is; InputStreamReader isr = new InputStreamReader(stream); BufferedReader rd = new BufferedReader(isr))
    {
      while (true)
      {
        String line = rd.readLine();
        if (line == null)
          break;
        if (line.startsWith("#"))
          continue;
        String[] parts = line.split("\\s+");
        ENTITIES.put(parts[0], Integer.parseInt(parts[1]));
      }
    } catch (IOException | NumberFormatException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  /**
   * Decode the HTML entities in a string.
   *
   * Named entities (<code>&amp;lt;</code>) and decimal or hexadecimal numeric
   * references (<code>&amp;#60;</code>, <code>&amp;#x3c;</code>) are replaced.
   * Anything that is not a well formed, known entity, such as a bare
   * ampersand, is left as written. This is used on documentation text, so it
   * must never throw.
   *
   * @param s is the text to decode.
   * @return the decoded text.
   */
  public static String decode(String s)
  {
    if (s.indexOf('&') < 0)
      return s;

    int n = s.length();
    StringBuilder sb = new StringBuilder(n);
    int i = 0;
    while (i < n)
    {
      char ch = s.charAt(i);
      if (ch != '&')
      {
        sb.append(ch);
        ++i;
        continue;
      }
      int end = decodeEntity(s, i, sb);
      if (end < 0)
      {
        sb.append('&');
        ++i;
      } else
        i = end;
    }
    return sb.toString();
  }

  /**
   * Decode the entity starting at the ampersand at <code>start</code>.
   *
   * @return the index just past the entity, or -1 if it is not a valid one
   * (nothing is appended in that case).
   */
  private static int decodeEntity(String s, int start, StringBuilder sb)
  {
    int n = s.length();
    int j = start + 1;
    if (j >= n)
      return -1;

    if (s.charAt(j) == '#')
    {
      ++j;
      int radix = 10;
      if (j < n && (s.charAt(j) == 'x' || s.charAt(j) == 'X'))
      {
        radix = 16;
        ++j;
      }
      int digits = j;
      long value = 0;
      while (j < n && Character.digit(s.charAt(j), radix) >= 0)
      {
        // Saturate so an absurdly long reference cannot overflow.
        if (value <= 0x10FFFF)
          value = value * radix + Character.digit(s.charAt(j), radix);
        ++j;
      }
      if (j == digits)
        return -1;
      // Be robust when the closing semicolon is missing.
      if (j < n && s.charAt(j) == ';')
        ++j;
      // NUL, surrogates and out of range values cannot be represented.
      if (value == 0 || value > 0x10FFFF
              || (value >= 0xD800 && value <= 0xDFFF))
        value = 0xFFFD;
      sb.appendCodePoint((int) value);
      return j;
    }

    int name = j;
    while (j < n && isEntityChar(s.charAt(j)))
      ++j;
    if (j == name || j >= n || s.charAt(j) != ';')
      return -1;
    Integer c = ENTITIES.get(s.substring(name, j));
    if (c == null)
      return -1;
    sb.appendCodePoint(c);
    return j + 1;
  }

  private static boolean isEntityChar(char c)
  {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9');
  }
//</editor-fold>
}
