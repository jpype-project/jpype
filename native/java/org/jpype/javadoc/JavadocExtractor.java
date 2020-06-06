package org.jpype.javadoc;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
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
  static public boolean failures = false;

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
      try (InputStream is = getDocumentationAsStream(cls))
      {
        if (is != null)
        {
          Parser<Document> parser = Html.newParser();
          return extractDocument(cls, parser.parse(is));
        }
      }
    } catch (Exception ex)
    {
      System.err.println("Failed to extract javadoc for " + cls);
      if (failures)
        throw new RuntimeException(ex);
    }
    return null;
  }

  public static InputStream getDocumentationAsStream(Class cls)
  {
    InputStream is = null;
    String name = cls.getName().replace('.', '/') + ".html";
    ClassLoader cl = ClassLoader.getSystemClassLoader();

    // Search the regular class path.
    is = cl.getResourceAsStream(name);
    if (is != null)
      return is;

    // Search for api documents
    String name1 = "docs/api/" + name;
    is = cl.getResourceAsStream(name1);
    if (is != null)
      return is;

    // If we are dealing with Java 9+, the doc tree is different
    try
    {
      Method meth = Class.class.getMethod("getModule");
      String module = meth.invoke(cls).toString().substring(7);
      String name2 = "docs/api/" + module + "/" + name;
      is = cl.getResourceAsStream(name2);
      if (is != null)
        return is;
    } catch (NoSuchMethodException | SecurityException | IllegalAccessException
            | IllegalArgumentException | InvocationTargetException ex)
    {
      // do nothing if we are not JDK 9+
    }
    return null;
  }

  /**
   * Extract the documentation from the dom.
   *
   * @param cls is the class being processed.
   * @param doc is the DOM holding the javadoc.
   * @return
   */
  public static Javadoc extractDocument(Class cls, Document doc)
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
          transformer.transformDescription(cls, description);
        if (render)
          documentation.description = renderer.render(description);
      }

      Node ctorRoot = ((Node) xPath.compile("//li/a[@name='constructor.detail' or @id='constructor.detail']")
              .evaluate(doc, XPathConstants.NODE));
      if (ctorRoot != null)
      {
        List<Node> set = convertNodes((NodeList) xPath.compile("./ul/li")
                .evaluate(ctorRoot.getParentNode(), XPathConstants.NODESET));
        documentation.ctorsNode = set;
        StringBuilder sb = new StringBuilder();
        for (Node ctor : set)
        {
          if (transform)
            transformer.transformMember(cls, ctor);
          if (render)
            sb.append(renderer.render(ctor));
        }
        documentation.ctors = sb.toString();
      }

      Node methodRoot = ((Node) xPath.compile("//li/a[@name='method.detail' or  @id='method.detail']")
              .evaluate(doc, XPathConstants.NODE));
      if (methodRoot != null)
      {
        List<Node> set = convertNodes((NodeList) xPath.compile("./ul/li")
                .evaluate(methodRoot.getParentNode(), XPathConstants.NODESET));
        documentation.methodNodes = set;
        for (Node method : set)
        {
          if (transform)
            transformer.transformMember(cls, method);
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
      Node fieldRoot = ((Node) xPath.compile("//li/a[@name='field.detail' or @id='field.detail']")
              .evaluate(doc, XPathConstants.NODE));
      if (fieldRoot != null)
      {
        List<Node> set = convertNodes((NodeList) xPath.compile("./ul/li")
                .evaluate(fieldRoot.getParentNode(), XPathConstants.NODESET));
        documentation.fieldNodes = set;
        for (Node field : set)
        {
          if (transform)
            transformer.transformMember(cls, field);
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
