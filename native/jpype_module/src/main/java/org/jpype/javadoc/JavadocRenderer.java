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
package org.jpype.javadoc;

import java.nio.charset.StandardCharsets;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import java.util.Map;
import java.util.HashMap;

/**
 * Render a node as ReStructured Text.
 *
 * @author nelson85
 */
public class JavadocRenderer
{

  static final int WIDTH = 120;
  public StringBuilder assembly;
  public int indentLevel = 0;
  String memberName;

  public String render(Node node)
  {
    try
    {
      indentLevel = 0;
      assembly = new StringBuilder();
      DomUtilities.traverseChildren(node, this::renderSections, Node.ELEMENT_NODE);
      return assembly.toString();
    } catch (Exception ex)
    {
      throw new JavadocException(node, ex);
    }
  }

  /**
   * Render the dom into restructured text.
   *
   * @param node
   */
  void renderSections(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    if (name.equals("title"))
    {
      this.memberName = node.getTextContent();
      return;
    }
    if (name.equals("signature"))
    {
      assembly.append(node.getTextContent())
              .append("\n\n");
      indentLevel += 4;
      return;
    }
    if (name.equals("description"))
    {
      renderText(node, true, true);
      return;
    }
    if (name.equals("details"))
    {
      DomUtilities.traverseChildren(node, this::renderDetails, Node.ELEMENT_NODE);
      assembly.append("\n");
      return;
    }
  }

  final static Map<String, String> SECTIONS = new HashMap<>();

  static
  {
    // Decide what sections to render
    SECTIONS.put("returns", "Returns:");
    SECTIONS.put("see", "See also:");
    SECTIONS.put("since", "Since:");
    SECTIONS.put("jls", "See Java\u2122 specification:");
    SECTIONS.put("overrides", "Overrides:");
    SECTIONS.put("specified", "Specified by:");
    SECTIONS.put("version", null);
    SECTIONS.put("typeparams", null);
    SECTIONS.put("author", null);
    SECTIONS.put("see", "Also see:");
    SECTIONS.put("api_note", "API Note:");
    SECTIONS.put("requirements", "Implementation Requirements:");
    SECTIONS.put("impl_note", "Implementation Note:");
  }

  void renderDetails(Node node)
  {
    String name = node.getNodeName();
    if (name.equals("parameters"))
    {
      assembly.append('\n')
              .append(indentation(this.indentLevel))
              .append("Parameters:\n");
      indentLevel += 4;
      DomUtilities.traverseChildren(node, this::renderParameter, Node.ELEMENT_NODE);
      indentLevel -= 4;
    } else if (name.equals("throws"))
    {
      assembly.append('\n')
              .append(indentation(this.indentLevel))
              .append("Raises:\n");
      indentLevel += 4;
      DomUtilities.traverseChildren(node, this::renderThrow, Node.ELEMENT_NODE);
      indentLevel -= 4;
    } else if (SECTIONS.containsKey(name))
    {
      String title = SECTIONS.get(name);
      if (title == null)
        return;
      assembly.append('\n')
              .append(indentation(this.indentLevel))
              .append(title).append('\n');
      indentLevel += 4;
      renderText(node, true, true);
      indentLevel -= 4;
    } else
    {
      System.err.println("Need renderer for section " + name);
    }
  }

  void renderParameter(Node node)
  {
    Element elem = (Element) node;
    assembly.append(indentation(this.indentLevel))
            //            .append("  ")
            .append(elem.getAttribute("name"))
            .append(" (")
            .append(elem.getAttribute("type"))
            .append("): ");
    indentLevel += 4;
    renderText(node, false, true);
    indentLevel -= 4;
  }

  void renderThrow(Node node)
  {
    Element elem = (Element) node;
    assembly.append(indentation(this.indentLevel))
            //.append("  ")
            .append(elem.getAttribute("name"))
            .append(": ");
    indentLevel += 4;
    renderText(node, false, true);
    indentLevel -= 4;
  }

  /**
   * Render a paragraph or paragraph like element.
   *
   */
  void renderText(Node node, boolean startIndent, boolean trailingNL)
  {
    DomUtilities.combineText(node);
    DomUtilities.removeWhitespace(node);
    Node child = node.getFirstChild();
    for (; child != null; child = child.getNextSibling())
    {
      if (child.getNodeType() == Node.TEXT_NODE)
      {
        String value = child.getNodeValue();
        if (value == null)
          continue;
        value = value.trim();
        if (value.isEmpty())
          continue;
        formatWidth(assembly, value, WIDTH, indentLevel, startIndent);
        if (trailingNL)
          assembly.append("\n");
        continue;
      }
      if (child.getNodeType() != Node.ELEMENT_NODE)
        continue;
      Element element = (Element) child;
      String name = element.getTagName();
      if (name.equals("p"))
      {
        assembly.append("\n");
        renderText(element, true, true);
      } else if (name.equals("div"))
      {
        renderText(element, true, true);
      } else if (name.equals("center"))
      {
        renderText(element, true, true);
      } else if (name.equals("br"))
      {
        assembly.append("\n\n");
      } else if (name.equals("ul"))
      {
        renderUnordered(element);
      } else if (name.equals("ol"))
      {
        renderOrdered(element);
      } else if (name.equals("img"))
      {
        // punt
      } else if (name.equals("table"))
      {
        // punt
      } else if (name.equals("hr"))
      {
        // punt
      } else if (name.equals("dl"))
      {
        renderDefinitions(element);
      } else if (name.equals("codeblock"))
      {
        renderCodeBlock(element);
      } else if (name.equals("blockquote"))
      {
        renderBlockQuote(element);
      } else if (name.equals("h1"))
      {
        renderHeader(element);
      } else if (name.equals("h2"))
      {
        renderHeader(element);
      } else if (name.equals("h3"))
      {
        renderHeader(element);
      } else if (name.equals("h4"))
      {
        renderHeader(element);
      } else if (name.equals("h5"))
      {
        renderHeader(element);
      } else
      {
        throw new RuntimeException("Need render for " + name);
      }
    }
  }

  void renderHeader(Node node)
  {
    assembly.append("\n");
    renderText(node, true, true);
    assembly.append(new String(new byte[node.getTextContent().length()]).replace('\0', '-'))
            .append("\n\n");
  }

  void renderBlockQuote(Node node)
  {
    indentLevel += 4;
    renderText(node, true, true);
    indentLevel -= 4;
  }

  /**
   * Render an unordered list.
   *
   * @param node
   */
  void renderOrdered(Node node)
  {
    indentLevel += 4;
    assembly.append("\n");
    Node child = node.getFirstChild();
    int num = 1;
    for (; child != null; child = child.getNextSibling())
    {
      if (child.getNodeType() != Node.ELEMENT_NODE)
        continue;
      if (child.getNodeName().equals("li"))
      {
        assembly.append(indentation(indentLevel - 2))
                .append(String.format("%d.  ", num++));
        renderText(child, false, true);
      } else
        throw new RuntimeException("Bad node " + child.getNodeName() + " in UL");
    }
    indentLevel -= 4;
    assembly.append("\n");
  }

  /**
   * Render an unordered list.
   *
   * @param node
   */
  void renderUnordered(Node node)
  {
    indentLevel += 4;
    assembly.append("\n");
    Node child = node.getFirstChild();
    for (; child != null; child = child.getNextSibling())
    {
      if (child.getNodeType() != Node.ELEMENT_NODE)
        continue;
      if (child.getNodeName().equals("li"))
      {
        assembly.append(indentation(indentLevel - 4))
                .append("  - ");
        renderText(child, false, true);
      } else
        throw new RuntimeException("Bad node " + child.getNodeName() + " in UL");
    }
    indentLevel -= 4;
    assembly.append("\n");
  }

  /**
   * Render a definition list.
   *
   * @param node
   */
  void renderDefinitions(Node node)
  {
    Node child = node.getFirstChild();
    for (; child != null; child = child.getNextSibling())
    {
      if (child.getNodeType() != Node.ELEMENT_NODE)
        continue;
      String name = child.getNodeName();
      if (name.equals("dt"))
      {
        assembly.append("\n");
        renderText(child, true, true);
      } else if (name.equals("dd"))
      {
        assembly.append(indentation(indentLevel));
        indentLevel += 4;
        assembly.append("  ");
        renderText(child, false, true);
        indentLevel -= 4;
      } else
        throw new RuntimeException("Bad node " + name + " in DL");
    }
    assembly.append("\n");
  }

  void renderCodeBlock(Node node)
  {
    String indent = indentation(indentLevel);
    assembly.append("\n")
            .append(indent)
            .append(".. code-block: java\n");
    String text = node.getTextContent();
    if (text.charAt(0) != '\n')
      text = "\n" + text;
    text = text.replaceAll("\n", "\n" + indent);
    assembly.append(indent).append(text).append("\n");
  }

//<editor-fold desc="text-utilities" defaultstate="collapsed">
  static final String SPACING = new String(new byte[40]).replace('\0', ' ');

  static String indentation(int level)
  {
    if (level > 40)
      return new String();
    return SPACING.substring(0, level);
  }

  static void formatWidth(StringBuilder sb, String s, int width, int indent, boolean flag)
  {
    String sindent = indentation(indent);
    s = s.replaceAll("\\s+", " ").trim();
    if (s.length() < width)
    {
      if (flag)
        sb.append(sindent);
      sb.append(s);
      return;
    }
    byte[] b = s.getBytes(StandardCharsets.UTF_8);
    int start = 0;
    int prev = 0;
    int l = b.length;
    int next = 0;
    while (next < l)
    {
      for (next = prev + 1; next < l; ++next)
        if (b[next] == ' ')
          break;
      if (next - start > width)
      {
        b[prev] = '\n';
        if (flag)
          sb.append(sindent);
        flag = true;
        sb.append(new String(b, start, prev - start + 1));
        start = prev + 1;
      }
      prev = next;
    }
    sb.append(sindent);
    sb.append(new String(b, start, l - start));
  }
//</editor-fold>
}
