package org.jpype.javadoc;

import java.nio.charset.StandardCharsets;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

/**
 * Render a node as ReStructured Text.
 *
 * @author nelson85
 */
public class JavadocRenderer
{

  public StringBuilder assembly;
  public int indentLevel = 0;
  String memberName;

  public String renderMember(Node node)
  {
    indentLevel = 0;
    assembly = new StringBuilder();
    DomUtilities.traverseChildren(node, this::renderMember, Node.ELEMENT_NODE);
    return assembly.toString();
  }

  /**
   * Render the dom into restructured text.
   *
   * @param node
   */
  public void renderSections(Node node)
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
      assembly.append(node.getTextContent());
      assembly.append("\n\n");
      return;
    }
    if (name.equals("description"))
    {
      renderText(node, true, true);
      return;
    }
    if (name.equals("details"))
    {
      renderDefinitions(node);
      return;
    }
  }

  /**
   * Render a paragraph or paragraph like element.
   *
   * @param node
   * @param startIndent
   * @param finish
   */
  public void renderText(Node node, boolean startIndent, boolean trailingNL)
  {
    Node child = node.getFirstChild();
    for (; child != null; child = child.getNextSibling())
    {
      if (child.getNodeType() == Node.TEXT_NODE)
      {
        String value = child.getNodeValue().trim();
        if (value.isEmpty())
          continue;
        formatWidth(assembly, value, 80, indentLevel, startIndent);
        if (trailingNL)
          assembly.append("\n");
        continue;
      }
      if (child.getNodeType() != Node.ELEMENT_NODE)
        continue;
      Element e = (Element) child;
      String name = e.getTagName();
      if (name.equals("p"))
      {
        assembly.append("\n");
        renderText(e, true, true);
        continue;
      }
      if (name.equals("center"))
      {
        renderText(e, true, true);
        continue;
      }
      if (name.equals("br"))
      {
        assembly.append("\n\n");
        continue;
      }
      if (name.equals("ul"))
      {
        renderUnordered(e);
        continue;
      }
      if (name.equals("ol"))
      {
        renderOrdered(e);
        continue;
      }
      if (name.equals("table"))
      {
        // punt
        continue;
      }
      if (name.equals("table"))
      {
        // punt
        continue;
      }
      if (name.equals("dl"))
      {
        renderDefinitions(e);
        continue;
      }
      if (name.equals("codeblock"))
      {
        renderCodeBlock(e);
        continue;
      }
      if (name.equals("blockquote"))
      {
        indentLevel += 4;
        renderText(e, true, true);
        indentLevel -= 4;
        continue;
      }
      throw new RuntimeException("Need render for " + name);
    }
  }

  /**
   * Render an unordered list.
   *
   * @param node
   */
  public void renderOrdered(Node node)
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
        assembly.append(new String(new byte[indentLevel - 2]).replace('\u0000', ' '));
        assembly.append(String.format("%d. ", num++));
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
  public void renderUnordered(Node node)
  {
    indentLevel += 2;
    assembly.append("\n");
    Node child = node.getFirstChild();
    for (; child != null; child = child.getNextSibling())
    {
      if (child.getNodeType() != Node.ELEMENT_NODE)
        continue;
      if (child.getNodeName().equals("li"))
      {
        assembly.append(new String(new byte[indentLevel - 2]).replace('\u0000', ' '));
        assembly.append("- ");
        renderText(child, false, true);
      } else
        throw new RuntimeException("Bad node " + child.getNodeName() + " in UL");
    }
    indentLevel -= 2;
    assembly.append("\n");
  }

  /**
   * Render a definition list.
   *
   * @param node
   */
  public void renderDefinitions(Node node)
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
        indentLevel += 4;
        renderText(child, true, true);
        indentLevel -= 4;
      } else
        throw new RuntimeException("Bad node " + name + " in DL");
    }
    assembly.append("\n");
  }

  public void renderCodeBlock(Node node)
  {
    assembly.append("\n");
    assembly.append(".. code-block: java\n");
    String text = node.getTextContent();
    String indent = indentation(indentLevel + 4);
    text.replaceAll("\n", "\n" + indent);
    if (text.charAt(0) != '\n')
      assembly.append("\n");
    assembly.append(indent);
    assembly.append(text);
    assembly.append("\n");
  }

//<editor-fold desc="text-utilities" defaultstate="collapsed">
  public static final String SPACING = new String(new byte[40]).replace('\0', ' ');

  public static String indentation(int level)
  {
    if (level > 40)
      return new String();
    return SPACING.substring(0, level);
  }
  public static void formatWidth(StringBuilder sb, String s, int width, int indent, boolean flag)
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
