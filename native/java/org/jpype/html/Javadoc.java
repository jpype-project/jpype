package org.jpype.html;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.ArrayList;
import java.util.function.Consumer;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;

public class Javadoc
{

  public Node description;
  public List<Node> ctors;
  public List<Node> methods;
  public List<Node> inner;
  public List<Node> fields;

  boolean first = false;

  public Node processNode(Node node)
  {
    first = true;
    traverse(node, Javadoc::fixEntities, Node.TEXT_NODE);
    traverse(node, this::pass1, Node.ELEMENT_NODE);
    traverse(node, this::pass2, Node.ELEMENT_NODE);
    return node;
  }

  public static void fixEntities(Node node)
  {
    Text n = (Text) node;
    String s = Html.decode(n.getTextContent());
    n.setTextContent(s);
  }

  final static String[] subs = new String[]
  {
    "cite", "\"%s\"",
    "code", "``%s``",
    "i", "*%s*",
    "b", "**%s**",
    "sup", ":superscript:`%s`",
    "sub", ":subscript:`%s`",
  };

  public void pass1(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();

    if (name.equals("pre") && first)
    {
      first = false;
      String s2 = e.getTextContent();
      s2 = s2.replaceAll("\\s+", " ");
      e.getParentNode().replaceChild(doc.createTextNode(String.format("%s\n", s2)), e);
      return;
    }

    if (name.equals("a"))
    {
      String href = e.getAttribute("href");
      if (href == null)
      {
        String s2 = e.getTextContent();
        e.getParentNode().replaceChild(doc.createTextNode(s2.trim()), e);
      } else
      {
        if (href.startsWith("../../"))
        {
          href = href.substring(6).replace('/', '.');
          if (href.contains("#"))
          {
            href = href.replaceAll("-.*", "");
            e.getParentNode().replaceChild(doc.createTextNode(String.format(":py:method:`%s`", href.trim())), e);
          } else
          {
            href = href.replaceAll("\\.html$", "");
            e.getParentNode().replaceChild(doc.createTextNode(String.format(":py:class:`%s`", href.trim())), e);
          }
        }
      }
    }

    for (int i = 0; i < subs.length; i += 2)
    {
      if (name.equals(subs[i]))
      {
        String s2 = e.getTextContent();
        e.getParentNode().replaceChild(doc.createTextNode(String.format(subs[i + 1], s2)), e);
        return;
      }
    }

    if (name.equals("span"))
    {
      String s2 = e.getTextContent();
      e.getParentNode().replaceChild(doc.createTextNode(String.format("%s", s2)), e);
      return;
    }

    if (name.equals("dt"))
    {
      String s2 = e.getTextContent();
      e.getParentNode().replaceChild(doc.createTextNode(String.format("%s", s2)), e);
      return;
    }

    if (name.equals("p"))
    {
      splitWidthNode(node, 70, 0);
      return;
    }

    if (name.equals("div"))
    {
      System.out.println("DIV");
      splitWidthNode(node, 70, 0);
      return;
    }

    if (name.equals("li"))
    {
      splitWidthNode(node, 70, 0);
      return;
    }

    if (name.equals("dd"))
    {
      String s2 = e.getTextContent();
      s2 = splitWidth(s2, 70, 4);
      e.getParentNode().replaceChild(doc.createTextNode(s2), e);
      return;
    }

  }

  public void pass2(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();

    if (name.equals("h4"))
    {
      String s2 = e.getTextContent();
      StringBuilder sb = new StringBuilder();
      sb.append(s2);
      sb.append("\n");
      sb.append(new String(new char[s2.length()]).replace('\0', '-'));
      Node p = e.getParentNode();
      p.replaceChild(doc.createTextNode(String.format(sb.toString(), s2)), e);
      return;
    }

    if (name.equals("pre"))
    {
      String s2 = e.getTextContent();
      Node p = e.getParentNode();
      p.replaceChild(doc.createTextNode(String.format("\n```\n%s\n```", s2)), e);
      return;
    }

    if (name.equals("div"))
    {
      String s2 = e.getTextContent();
      Node p = e.getParentNode();
      p.replaceChild(doc.createTextNode(s2), e);
      return;
    }

    if (name.equals("dl"))
    {
      String s2 = e.getTextContent();
      Node p = e.getParentNode();
      p.replaceChild(doc.createTextNode(s2), e);
      return;
    }
  }

  public static void splitWidthNode(Node node, int width, int indent)
  {
    // merge text nodes
    Node child = node.getFirstChild();
    while (child != null)
    {
      Node next = child.getNextSibling();
      if (child.getNodeType() != Node.TEXT_NODE)
      {
        child = next;
        continue;
      }
      if (next != null && next.getNodeType() == Node.TEXT_NODE)
      {
        child.setTextContent(child.getTextContent() + next.getTextContent());
        child.getParentNode().removeChild(next);
        continue;
      }
      child = next;
    }

    NodeList nodeList = node.getChildNodes();
    for (int i = 0; i < nodeList.getLength(); i++)
    {
      Node currentNode = nodeList.item(i);
      if (currentNode.getNodeType() == Node.TEXT_NODE)
      {
        Text text = (Text) currentNode;
        String s = text.getTextContent();
        s = splitWidth(s, width, indent);
        text.setTextContent(s);
      }
    }
  }

  public static String splitWidth(String s, int width, int indent)
  {
    String sindent = new String(new char[indent]).replace('\0', ' ');
    s = s.replaceAll("\\s+", " ");
    if (s.length() < width)
      return sindent + s;
    byte[] b = s.getBytes(StandardCharsets.UTF_8);
    int start = 0;
    int prev = 0;
    int l = b.length;
    int next = 0;

    List<String> strings = new ArrayList<>();
    strings.add(new String());

    while (next < l)
    {
      for (next = prev + 1; next < l; ++next)
        if (b[next] == ' ')
          break;
      if (next - start > width)
      {
        b[prev] = '\n';
        strings.add(new String(b, start, prev - start + 1));
        start = prev + 1;
      }
      prev = next;
    }
    strings.add(new String(b, start, l - start));
    return String.join(sindent, strings);
  }

  public static void traverse(Node node, Consumer<Node> operator, short type)
  {
    NodeList nodeList = node.getChildNodes();
    for (int i = 0; i < nodeList.getLength(); i++)
    {
      Node currentNode = nodeList.item(i);
      // Apply transforms to children first
      if (currentNode.getNodeType() == Node.ELEMENT_NODE)
        traverse(currentNode, operator, type);
      // Then process the outer element
      if (currentNode.getNodeType() == type)
        operator.accept(currentNode);
    }
  }
}
