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
    ClassLoader cl = ClassLoader.getSystemClassLoader();
    try (InputStream is = cl.getResourceAsStream("org/jpype/html/entities.txt"); InputStreamReader isr = new InputStreamReader(is); BufferedReader rd = new BufferedReader(isr))
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
    } catch (IOException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  public static String decode(String s)
  {
    if (!s.contains("&"))
      return s;

    int dead = 0;
    byte[] b = s.getBytes(StandardCharsets.UTF_8);
    for (int i = 0; i < b.length; ++i)
    {
      if (b[i] != '&')
        continue;

      int i1 = i;
      int i2 = i + 1;
      if (i2 == b.length)
        break;
      if (b[i2] == '#')
      {
        // Try to be robust when there is no ;
        for (i = i2 + 1; i < b.length; ++i)
        {
          if (!Character.isDigit(b[i]))
            break;
        }
      } else
      {
        for (i = i2; i < b.length; ++i)
        {
          if (b[i] == ';')
            break;
        }
      }
      int i3 = i;
      int c = 0;
      if (b[i2] == '#')
      {
        i2++;
        try
        {
          c = Integer.parseInt(new String(b, i2, i3 - i2, StandardCharsets.UTF_8));
        } catch (NumberFormatException ex)
        {

        }
      } else
      {
        String e = new String(b, i2, i3 - i2, StandardCharsets.UTF_8);
        Integer c2 = ENTITIES.get(e);
        if (c2 == null)
          throw new RuntimeException("Bad entity " + e);
        c = c2;
      }

      // Substitute
      if (c < 128)
      {
        b[i1++] = (byte) c;
      } else if (c < 0x0800)
      {
        b[i1++] = (byte) (0xc0 + ((c >> 6) & 0x1f));
        if (i1 < b.length) // lgtm [java/constant-comparison]
          b[i1++] = (byte) (0x80 + (c & 0x3f)); // lgtm [java/index-out-of-bounds]
      } else
      {
        b[i1++] = (byte) (0xe0 + ((c >> 12) & 0x0f));
        if (i1 < b.length) // lgtm [java/constant-comparison]
          b[i1++] = (byte) (0x80 + ((c >> 6) & 0x3f)); // lgtm [java/index-out-of-bounds]
        if (i1 < b.length)
          b[i1++] = (byte) (0x80 + (c & 0x3f));
      }
      if (i3 < b.length && b[i3] == ';')
        i3++;
      dead += i3 - i1;
      for (; i1 < i3; ++i1)
        b[i1] = 0;
      i = i3;
    }
    int j = 0;
    byte[] b2 = new byte[b.length - dead];
    for (int i = 0; i < b.length; ++i)
    {
      if (b[i] != 0)
        b2[j++] = b[i];
    }
    return new String(b2, StandardCharsets.UTF_8);
  }
//</editor-fold>
}
