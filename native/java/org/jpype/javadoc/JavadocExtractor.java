package org.jpype.javadoc;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.jpype.html.Html;
import org.jpype.html.Parser;
import org.w3c.dom.Document;
import org.w3c.dom.DocumentFragment;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class JavadocExtractor
{

  static final JavadocTransformer transformer = new JavadocTransformer();
  static public boolean transform = true;
  static public boolean render = true;

  /**
   * Search the classpath for documentation.
   *
   * @param cls
   * @return
   */
  public static Javadoc getDocumentation(Class cls)
  {
    try
    {
      String name = cls.getName().replace('.', '/') + ".html";
      ClassLoader cl = ClassLoader.getSystemClassLoader();

      // Search the regular class path.
      try (InputStream is = cl.getResourceAsStream(name))
      {
        if (is != null)
          return extractStream(is);
      }

      // Search for api documents
      name = "docs/api/" + name;
      try (InputStream is = cl.getResourceAsStream(name))
      {
        if (is != null)
          return extractStream(is);
      }
    } catch (IOException ex)
    {
      System.out.println("Failed to extract javadoc");
      throw new RuntimeException(ex);
    }
    return null;
  }

  /**
   * Extract the documentation from a Path.
   *
   * @param path
   * @return
   */
  public static Javadoc extractPath(Path path)
  {
    try (InputStream is = Files.newInputStream(path))
    {
      Parser<Document> parser = Html.newParser();
      return extractDocument(parser.parse(is));
    } catch (Exception ex)
    {
      return null;
    }
  }

  /**
   * Extract the documentation from a stream.
   *
   * @param is
   * @return
   */
  public static Javadoc extractStream(InputStream is)
  {
    Parser<Document> parser = Html.newParser();
    return extractDocument(parser.parse(is));
  }

  /**
   * Extract the documentation from the dom.
   *
   * @param doc
   * @return
   */
  public static Javadoc extractDocument(Document doc)
  {
    JavadocRenderer renderer = new JavadocRenderer();
    try
    {
      Javadoc documentation = new Javadoc();
      XPath xPath = XPathFactory.newInstance().newXPath();
      Node description = toFragment((Node) xPath.compile("//div[@class='description']/ul/li").evaluate(doc, XPathConstants.NODE));
      if (description != null)
      {
        documentation.descriptionNode = description;
        if (transform)
          transformer.transformDescription(description);
        if (render)
          documentation.description = renderer.render(description);
      }

      Node ctorRoot = ((Node) xPath.compile("//li/a[@name='constructor.detail']")
              .evaluate(doc, XPathConstants.NODE)).getParentNode();
      if (ctorRoot != null)
      {
        List<Node> set = convertNodes((NodeList) xPath.compile("./ul/li")
                .evaluate(ctorRoot, XPathConstants.NODESET));
        documentation.ctorsNode = set;
        StringBuilder sb = new StringBuilder();
        for (Node ctor : set)
        {
          if (transform)
            transformer.transformMember(ctor);
          if (render)
            sb.append(renderer.render(ctor));
        }
        documentation.ctors = sb.toString();
      }

      Node methodRoot = ((Node) xPath.compile("//li/a[@name='method.detail']")
              .evaluate(doc, XPathConstants.NODE)).getParentNode();
      if (methodRoot != null)
      {
        List<Node> set = convertNodes((NodeList) xPath.compile("./ul/li")
                .evaluate(methodRoot, XPathConstants.NODESET));
        documentation.methodNodes = set;
        for (Node method : set)
        {
          if (transform)
            transformer.transformMember(method);
          if (render)
          {
            String str = renderer.render(method);
            String name = renderer.memberName;
            if (documentation.methods.containsKey(name))
            {
              String old = documentation.methods.get(name);
              str = old + str;
            }
            documentation.methods.put(name, str);
          }
        }
      }

//      Node inner = (Node) xPath.compile("//li/a[@name='nested_class_summary']").evaluate(doc, XPathConstants.NODE);
//      if (inner != nullList)
//      {
//        NodeList set = (NodeList) xPath.compile("./ul/li").evaluate(inner.getParentNode(), XPathConstants.NODESET);
//        documentation.innerNode = convertNodes(set);
//      }
      Node fieldRoot = ((Node) xPath.compile("//li/a[@name='field.detail']")
              .evaluate(doc, XPathConstants.NODE)).getParentNode();
      if (fieldRoot != null)
      {
        List<Node> set = convertNodes((NodeList) xPath.compile("./ul/li")
                .evaluate(fieldRoot, XPathConstants.NODESET));
        documentation.fieldNodes = set;
        for (Node field : set)
        {
          if (transform)
            transformer.transformMember(field);
          if (render)
          {
            String str = renderer.render(field);
            String name = renderer.memberName;
            documentation.fields.put(name, str);
          }
        }
      }

      return documentation;
    } catch (IOException | XPathExpressionException ex)
    {
      throw new RuntimeException(ex);
//      return null;
    }
  }

  private static List<Node> convertNodes(NodeList nl) throws IOException
  {
    List<Node> out = new ArrayList<>();
    for (int i = 0; i < nl.getLength(); ++i)
    {
      out.add(toFragment(nl.item(i)));
    }
    return out;
  }

  /**
   * Convert a portion of the document into a fragment.
   *
   * @param node
   * @return
   */
  public static Node toFragment(Node node)
  {
    Document doc = node.getOwnerDocument();
    DocumentFragment out = doc.createDocumentFragment();
    while (node.hasChildNodes())
    {
      out.appendChild(node.getFirstChild());
    }
    if (out.getFirstChild() != null && out.getFirstChild().getNodeType() == Node.TEXT_NODE)
      out.removeChild(out.getFirstChild());
    if (out.getLastChild() != null && out.getLastChild().getNodeType() == Node.TEXT_NODE)
      out.removeChild(out.getLastChild());
    return out;
  }
}
