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

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.jpype.html.Html;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
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

  final static Pattern ARGS_PATTERN = Pattern.compile(".*\\((.*)\\).*");

  public Node transformDescription(Class cls, Node node)
  {
    try
    {
      Workspace ws = new Workspace(cls);
      DomUtilities.traverseDFS(node, this::fixEntities, Node.TEXT_NODE);
      DomUtilities.traverseChildren(node, this::handleDescription, Node.ELEMENT_NODE, ws);
      DomUtilities.traverseDFS(node, this::pass1, Node.ELEMENT_NODE, ws);
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
  public Node transformMember(Class cls, Node node)
  {
    try
    {
      Workspace ws = new Workspace(cls);
      DomUtilities.traverseDFS(node, this::fixEntities, Node.TEXT_NODE);
      DomUtilities.traverseChildren(node, this::handleMembers, Node.ELEMENT_NODE, ws);
      DomUtilities.traverseDFS(node, this::pass1, Node.ELEMENT_NODE, ws);
      return node;
    } catch (Exception ex)
    {
      throw new JavadocException(node, ex);
    }

  }

//<editor-fold desc="members" defaultstate="description">
  void handleDescription(Node node, Workspace data)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();
    Node parent = node.getParentNode();
    if (name.equals("dl") && !data.hr)
    {
      parent.removeChild(node);
    } else if (name.equals("br"))
    {
      parent.removeChild(node);
    } else if (name.equals("hr"))
    {
      data.hr = true;
      parent.removeChild(node);
    } else if (name.equals("pre")
            || // Javadoc pre-17
            (name.equals("div") && e.getAttribute("class").equals("type-signature"))) // Javadoc 17+
    {
      DomUtilities.removeWhitespace(node);
      doc.renameNode(node, null, "signature");
    } else if (name.equals("div"))
    {
      doc.renameNode(node, null, "description");
      DomUtilities.clearAttributes(node);
    } else if (name.equals("dl"))
    {
      doc.renameNode(node, null, "details");
      DomUtilities.traverseChildren(node, this::handleDetails,
              Node.ELEMENT_NODE, data);
    } else
    {
      throw new RuntimeException("Unknown item at top level " + name);
    }
  }

//</editor-fold>
//<editor-fold desc="members" defaultstate="collapsed">
  void handleMembers(Node node, Workspace ws)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();

    if (name.equals("h4") || name.equals("h3")) // h4 for Javadoc pre-17, h3 for Javadoc 17+
    {
      doc.renameNode(node, null, "title");
    } else if (name.equals("pre")
            || // Javadoc pre-17
            (name.equals("div") && (e.getAttribute("class").equals("member-signature")))) // Javadoc 17+
    {
      doc.renameNode(node, null, "signature");
      DomUtilities.traverseDFS(node, this::pass1, Node.ELEMENT_NODE, ws);
      // We need to get the types from here for the parameters
      DomUtilities.removeWhitespace(node);
      String content = node.getTextContent();
      Matcher m = ARGS_PATTERN.matcher(content);
      if (m.matches())
      {
        LinkedList<String> types = new LinkedList<>();
        for (String s : m.group(1).split(", "))
        {
          String[] parts = s.split("\u00a0", 2);
          types.add(parts[0]);
        }
        ws.types = types;
      }
    } else if (name.equals("div"))
    {
      doc.renameNode(node, null, "description");
      DomUtilities.clearAttributes(node);
    } else if (name.equals("dl"))
    {
      doc.renameNode(node, null, "details");
      DomUtilities.traverseChildren(node, this::handleDetails,
              Node.ELEMENT_NODE, ws);
    } else
    {
      throw new RuntimeException("Unknown item at top level " + name);
    }
  }

  public final static Map<String, String> DETAIL_SECTIONS;

  static
  {
    DETAIL_SECTIONS = new HashMap<>();
    Map<String, String> ds = DETAIL_SECTIONS;
    ds.put("Author:", "author");
    ds.put("Since:", "since");
    ds.put("Parameters:", "parameters");
    ds.put("Returns:", "returns");
    ds.put("Overrides:", "overrides");
    ds.put("See Also:", "see");
    ds.put("API Note:", "api_note");
    ds.put("Version:", "version");
    ds.put("Type Parameters:", "typeparams");
    ds.put("Specified by:", "specified");
    ds.put("Throws:", "throws");
    ds.put("Implementation Requirements:", "requirements");
    ds.put("Implementation Note:", "impl_note");
  }

  void handleDetails(Node node, Workspace ws)
  {
    Element e = (Element) node;
    String name = e.getTagName();
    Document doc = e.getOwnerDocument();
    Node parent = e.getParentNode();
    if (name.equals("dt"))
    {
      String key = node.getTextContent().trim();
      if (DETAIL_SECTIONS.containsKey(key))
      {
        doc.renameNode(node, null, DETAIL_SECTIONS.get(key));
      } else if (key.startsWith("See "))
      {
        doc.renameNode(node, null, "jls");
      } else
      {
        System.err.println("Bad detail key '" + key + "'");
      }
      ws.key = node.getNodeName();
      ws.section = node;
      DomUtilities.clearChildren(ws.section);
    }
    if (name.equals("dd"))
    {
      if (ws.key.equals("parameters"))
      {
        Node first = node.getFirstChild(); // First is <code>varname</code>
        Node second = first.getNextSibling(); // Second is " - desc"
        Element elem = doc.createElement("parameter");
        elem.setAttribute("name", first.getTextContent());
        elem.setAttribute("type", ws.types.removeFirst());
        String value = second.getNodeValue();
        second.setNodeValue(value.substring(3)); // Remove " - "
        node.removeChild(first);
        DomUtilities.transferContents(elem, node);
        ws.section.appendChild(elem);
        parent.removeChild(node);
      } else if (ws.key.equals("throws"))
      {
        Node first = node.getFirstChild(); // First is <code><a>exc</a></code>
        Node second = first.getNextSibling(); // Second is " - desc"
        Element elem = doc.createElement("exception");
        DomUtilities.traverseDFS(first, this::pass1, Node.ELEMENT_NODE, ws);
        elem.setAttribute("name", first.getTextContent());
        if (second != null)
        {
          String value = second.getNodeValue();
          second.setNodeValue(value.substring(3)); // Remove " - "
        }
        node.removeChild(first);
        DomUtilities.transferContents(elem, node);
        ws.section.appendChild(elem);
        parent.removeChild(node);
      } else
      {
        // Normalize the node and transfer it to the section
        DomUtilities.transferContents(ws.section, node);
        DomUtilities.traverseDFS(ws.section, this::pass1, Node.ELEMENT_NODE, ws);
        DomUtilities.removeWhitespace(ws.section);
        parent.removeChild(node);
        return;
      }
    }
  }

  static class Workspace
  {

    private final Class cls;
    boolean hr = false;
    String key;
    Node section;
    private LinkedList<String> types;

    Workspace(Class cls)
    {
      this.cls = cls;
    }
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
    "pre", ":code:`%s`",
    "tt", "``%s``",
    "i", "*%s*",
    "em", "*%s*",
    "strong", "**%s**",
    "b", "**%s**",
    "sup", " :sup:`%s` ",
    "sub", " :sub:`%s` ",
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
  void pass1(Node node, Workspace ws)
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
      if (DomUtilities.containsNL(e))
      {
        doc.renameNode(node, null, "codeblock");
        name = "codeblock";
      } else
      {
        doc.renameNode(node, null, "code");
        name = "code";
      }
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
      String href = this.toReference(ws, eparent.getAttribute("href"));
      DomUtilities.clearChildren(parent);
      parent.appendChild(doc.createTextNode(href));
      return;
    }

    // <code><a> is also used.
    if (name.equals("a") && parent.getNodeName().equals("code"))
    {
      String href = this.toReference(ws, e.getAttribute("href"));
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
      href = this.toReference(ws, href);
      if (href == null)
        parent.replaceChild(doc.createTextNode(node.getTextContent()), node);
      else
        parent.replaceChild(doc.createTextNode(href), node);
      return;
    }

    // Apply inline transformations.
    for (int i = 0; i < PASS1.length; i += 2)
    {
      if (name.equals(PASS1[i]))
      {
        String s2 = e.getTextContent();
        if (s2 == null)
          e.getParentNode().removeChild(e);
        else
          e.getParentNode().replaceChild(doc.createTextNode(String.format(PASS1[i + 1], s2.trim())), e);
        return;
      }
    }

  }

  /**
   * Convert a reference into method or class.
   *
   * This currently only deals with local links. External links are elsewhere.
   *
   * @param ws
   * @param href
   * @return
   */
  public String toReference(Workspace ws, String href)
  {
    try
    {
      Path p = Paths.get(ws.cls.getName().replace('.', '/'));
      if (href.startsWith("#"))
      {
        // technically it may be a field, but we can't tell currently.
        Path q = p.resolve(href.substring(1).trim())
                .normalize();
        if (q.startsWith(".."))
          q = q.subpath(2, q.getNameCount());
        String r = q.toString()
                .replace('/', '.')
                .replace('\\', '.')
                .replaceAll("\\(.*\\)", "")
                .replaceAll("-.*", "");
        return String.format(":meth:`~%s`", r);
      } else if (href.contains("#"))
      {
        // technically it may be a field, but we can't tell currently.
        Path q = p.getParent()
                .resolve(href.trim())
                .normalize();
        if (q.startsWith(".."))
          q = q.subpath(2, q.getNameCount());
        String r = q.toString()
                .replace('/', '.')
                .replace('\\', '.')
                .replaceAll("\\(.*\\)", "")
                .replaceAll("-.*", "")
                .replaceAll(".html#", ".");
        return String.format(":meth:`~%s`", r);
      } else
      {
        Path q = p.getParent()
                .resolve(href.trim())
                .normalize();
        if (q.startsWith(".."))
          q = q.subpath(2, q.getNameCount());
        String r = q.toString()
                .replace('/', '.')
                .replace('\\', '.')
                .replaceAll("-.*", "")
                .replaceAll(".html", "");
        return String.format(":class:`~%s`", r);
      }
    } catch (Exception ex)
    {
      // There is a lot of ways this can go wrong.  If all else fails
      // return null so that we can just remove the hyperlink.
      return null;
    }
  }

//</editor-fold>
}
