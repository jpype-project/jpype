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
public class JavadocTransformer
{

  public Node transformDescription(Node node)
  {
    try
    {
      DomUtilities.traverseDFS(node, this::fixEntities, Node.TEXT_NODE);
      DomUtilities.traverseChildren(node, this::descriptionTop, Node.ELEMENT_NODE, new DescData());
      DomUtilities.traverseDFS(node, this::pass1, Node.ELEMENT_NODE);
      return node;
    } catch (Exception ex)
    {
      throw new JavadocException(node, ex);
    }
  }

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
    try
    {
      DomUtilities.traverseChildren(node, this::membersTop, Node.ELEMENT_NODE);
      DomUtilities.traverseDFS(node, this::fixEntities, Node.TEXT_NODE);
      DomUtilities.traverseDFS(node, this::pass1, Node.ELEMENT_NODE);
      return node;
    } catch (Exception ex)
    {
      throw new JavadocException(node, ex);
    }

  }

//<editor-fold desc="members" defaultstate="description">
  void descriptionTop(Node node, DescData d)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();
    Node parent = node.getParentNode();
    if (name.equals("dl") && !d.hr)
    {
      parent.removeChild(node);
    } else if (name.equals("br"))
    {
      parent.removeChild(node);
    } else if (name.equals("hr"))
    {
      d.hr = true;
      parent.removeChild(node);
    } else if (name.equals("pre"))
    {
      removeWhitespace(node);
      doc.renameNode(node, null, "signature");
    } else if (name.equals("div"))
    {
      doc.renameNode(node, null, "description");
      DomUtilities.clearAttributes(node);
    } else if (name.equals("dl"))
    {
      doc.renameNode(node, null, "details");
    } else
    {
      throw new RuntimeException("Unknown item at top level " + name);
    }
  }

  static class DescData
  {

    boolean hr = false;
  }

//</editor-fold>
//<editor-fold desc="members" defaultstate="collapsed">
  void membersTop(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();

    if (name.equals("h4"))
    {
      doc.renameNode(node, null, "title");
    } else if (name.equals("pre"))
    {
      removeWhitespace(node);
      doc.renameNode(node, null, "signature");
    } else if (name.equals("div"))
    {
      doc.renameNode(node, null, "description");
      DomUtilities.clearAttributes(node);
    } else if (name.equals("dl"))
    {
      doc.renameNode(node, null, "details");
      DomUtilities.traverseChildren(node, this::memberDetails,
              Node.ELEMENT_NODE, new DetailData());
    } else
    {
      throw new RuntimeException("Unknown item at top level " + name);
    }
  }

  void memberDetails(Node node, DetailData data)
  {
    System.out.println("DETAILS " + node.getNodeName());
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();
    Node parent = e.getParentNode();
    if (name.equals("dt"))
    {
      String key = node.getTextContent().trim();
      if (key.equals("Since:"))
      {
        doc.renameNode(node, null, "since");
      } else if (key.equals("Parameters:"))
      {
        doc.renameNode(node, null, "parameters");
      } else if (key.equals("Returns:"))
      {
        doc.renameNode(node, null, "returns");
      } else if (key.equals("Overrides:"))
      {
        doc.renameNode(node, null, "overrides");
      } else if (key.equals("See Also:"))
      {
        doc.renameNode(node, null, "see");
      } else if (key.startsWith("See "))
      {
        doc.renameNode(node, null, "jls");
      } else if (key.equals("Specified by:"))
      {
        doc.renameNode(node, null, "specified");
      } else if (key.equals("Throws:"))
      {
        doc.renameNode(node, null, "throws");
      } else
      {
        throw new RuntimeException("Bad detail key '" + key + "'");
      }
      data.key = node.getNodeName();
      data.section = node;
      DomUtilities.clearChildren(data.section);
    }
    if (name.equals("dd"))
    {
      System.out.println("TRANSFORM " + data.key);
      if (data.key.equals("since") || data.key.equals("returns") || data.key.equals("jls")
              || data.key.equals("see") || data.key.equals("overrides"))
      {
        DomUtilities.transferContents(data.section, node);
        parent.removeChild(node);
        return;
      }
      if (data.key.equals("parameters"))
      {
        Node first = node.getFirstChild(); // First is <code>varname</code>
        Node second = first.getNextSibling(); // Second is " - desc"
        Element elem = doc.createElement("parameter");
        elem.setAttribute("name", first.getTextContent());
        String value = second.getNodeValue();
        second.setNodeValue(value.substring(3)); // Remove " - "
        node.removeChild(first);
        DomUtilities.transferContents(elem, node);
        data.section.appendChild(elem);
        parent.removeChild(node);
      }
      if (data.key.equals("throws"))
      {
        Node first = node.getFirstChild(); // First is <code><a>exc</a></code>
        Node second = first.getNextSibling(); // Second is " - desc"
        Element elem = doc.createElement("exception");
        DomUtilities.traverseDFS(first, this::pass1, Node.ELEMENT_NODE);
        elem.setAttribute("name", first.getTextContent());
        String value = second.getNodeValue();
        second.setNodeValue(value.substring(3)); // Remove " - "
        node.removeChild(first);
        DomUtilities.transferContents(elem, node);
        data.section.appendChild(elem);
        parent.removeChild(node);
      }
    }
  }

  static class DetailData
  {

    String key;
    Node section;
  }

//</editor-fold>
//<editor-fold desc="contents" defaultstate="collapsed">
  /**
   * Convert any html entities found the text.
   *
   * @param node
   */
  void fixEntities(Node node)
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
    "strong", "**%s**",
    "b", "**%s**",
    "sup", ":sup:`%s`",
    "sub", ":sub:`%s`",
    "small", ":sub:`%s`",
    "span", "%s",
    "nop", "%s",
    "font", "%s",
    "var", "*%s*",
  };

  /**
   * Get a bunch of simple substitutions.
   *
   * @param node
   */
  void pass1(Node node)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();
    Node parent = e.getParentNode();
    if (parent == null)
      return;

    // Pre is something used to mark code.
    if (name.equals("pre"))
    {
      doc.renameNode(parent, null, "code");
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
  static void removeWhitespace(Node node)
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
      return String.format(":meth:`~%s`", href.trim());
    } else
    {
      href = href.replaceAll("\\.html$", "");
      return String.format(":class:`~%s`", href.trim());
    }
  }
//</editor-fold>
}
