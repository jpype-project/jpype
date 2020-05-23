package org.jpype.javadoc;

import org.jpype.html.Html;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;

/**
 * Transform the document into a form suitable for ReStructured Text.
 *
 * The goal of this is to convert all inline markup into rst and leave markup by
 * section, paragraph to be used by the renderer.
 *
 * @author nelson85
 */

public class JavadocFormatter
{

  public String memberName;

  /**
   * Convert a Javadoc member description into markup for ReStructure Text
   * rendering.
   *
   * This will mutilate the node.
   *
   * @param node
   */
  public Node transformMember(Node node)
  {
    DomUtilities.traverseChildren(node, this::membersTop, Node.ELEMENT_NODE);
    DomUtilities.traverseDFS(node, this::fixEntities, Node.TEXT_NODE);
    DomUtilities.traverseDFS(node, this::pass1, Node.ELEMENT_NODE);
    return node;
  }

//<editor-fold desc="members" defaultstate="collapsed">
  public void membersTop(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();

    if (name.equals("h4"))
    {
      doc.renameNode(node, null, "title");
    }
    else if (name.equals("pre"))
    {
      removeWhitespace(node);
      doc.renameNode(node, null, "signature");
    }
    else if (name.equals("div"))
    {
      doc.renameNode(node, null, "description");
      DomUtilities.clearAttributes(node);
    }
    else if (name.equals("dl"))
    {
      doc.renameNode(node, null, "details");
    } else
    {
      throw new RuntimeException("Unknown item at top level " + name);
    }
  }

  /**
   * Convert any html entities found the text.
   *
   * @param node
   */
  public void fixEntities(Node node)
  {
    Text n = (Text) node;
    String s = Html.decode(n.getTextContent());
    n.setTextContent(s);
  }

  // This corresponds to any simple inline markup transformation.
  final static String[] PASS1 = new String[]
  {
    "cite", "\"%s\"",
    "code", ":code:`%s`",
    "tt", "``%s``",
    "i", "*%s*",
    "em", "*%s*",
    "b", "**%s**",
    "sup", ":sup:`%s`",
    "sub", ":sub:`%s`",
    "small", ":sub:`%s`",
    "span", "%s",
    "nop", "%s",
    "var", "*%s*",
  };

  /**
   * Get a bunch of simple substitutions.
   *
   * @param node
   */
  public void pass1(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();
    Node parent = e.getParentNode();
    if (parent == null)
      return;

    // Pre is something used to mark code.
    if (name.equals("pre") && DomUtilities.containsNL(node))
    {
      name = "code";
    }

    //  <pre><code> is a common javadoc idiom.
    if (name.equals("code") && (parent.getNodeName().equals("pre")
            || parent.getNodeName().equals("blockquote")))
    {
      doc.renameNode(parent, null, "codeblock");
      DomUtilities.mergeNode(parent, node);
      return;
    }

    if (name.equals("codeblock") && parent.getNodeName().equals("pre"))
    {
      if (DomUtilities.containsNL(node))
      {
        doc.renameNode(parent, null, "codeblock");
      } else
        doc.renameNode(parent, null, "code");
      DomUtilities.mergeNode(parent, node);
      return;
    }

    // <a ...><code> is used to reference members and classes.
    if (name.equals("code") && parent.getNodeName().equals("a"))
    {
      Element eparent = (Element) parent;
      String href = this.toReference(eparent.getAttribute("href"));
      DomUtilities.clearChildren(parent);
      parent.appendChild(doc.createTextNode(href));
      return;
    }

    // <code><a> is also used.
    if (name.equals("a") && parent.getNodeName().equals("code"))
    {
      String href = this.toReference(e.getAttribute("href"));
      DomUtilities.clearChildren(parent);
      doc.renameNode(parent, null, "nop");
      parent.appendChild(doc.createTextNode(href));
      return;
    }

    // <a> by itself is usually external references.
    if (name.equals("a"))
    {
      String href = e.getAttribute("href");
      if (href.startsWith("http:") || href.startsWith("shttp:"))
      {
        String content = node.getTextContent();
        content = String.format("`%s <%s>`", content, href);
        parent.replaceChild(doc.createTextNode(content), node);
        return;
      }
      if (!href.startsWith(".."))
      {
        parent.replaceChild(doc.createTextNode(node.getTextContent()), node);
        return;
      }
      href = this.toReference(href);
      parent.replaceChild(doc.createTextNode(href), node);
      return;
    }

    // Apply inline transformations.
    for (int i = 0; i < PASS1.length; i += 2)
    {
      if (name.equals(PASS1[i]))
      {
        String s2 = e.getTextContent();
        e.getParentNode().replaceChild(doc.createTextNode(String.format(PASS1[i + 1], s2.trim())), e);
        return;
      }
    }

    // These elements need to be stripped of whitespace.
    if (name.equals("dd") || name.equals("p")
            || name.equals("description") || name.equals("li")
            || name.equals("dt"))
    {
      DomUtilities.combineText(node);
      removeWhitespace(node);
    }
  }

  /**
   * Traverse a node and replaces all extra whitespace with one space.
   *
   * This should be applied to any element where white space is not relevant.
   *
   * @param node
   */
  public static void removeWhitespace(Node node)
  {
    // merge text nodes
    NodeList children = node.getChildNodes();
    for (int i = 0; i < children.getLength(); ++i)
    {
      Node child = children.item(i);
      if (child.getNodeType() != Node.TEXT_NODE)
        continue;
      Text t = (Text) child;
      String c = t.getNodeValue();
      c = c.replaceAll("\\s+", " ");
      t.setNodeValue(c);
    }
  }

  /**
   * Convert a reference into method or class.
   *
   * @param href
   * @return
   */
  public String toReference(String href)
  {
    href = href.replaceAll("\\.\\.\\/", "");
    href = href.replace('/', '.');
    if (href.contains("#"))
    {
      // technically it may be a field, but we can't tell currently.
      href = href.replaceAll("-.*", "");
      href = href.replace(".html#", ".");
      return String.format(":method:`~%s`", href.trim());
    } else
    {
      href = href.replaceAll("\\.html$", "");
      return String.format(":class:`~%s`", href.trim());
    }
  }
}
