package org.jpype.html;

import java.io.ByteArrayOutputStream;
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
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class JavadocExtractor
{

  public static Javadoc extract(Path path)
  {
    Document doc;
    try (InputStream is = Files.newInputStream(path))
    {
      Parser<Document> parser = Html.newParser();
      return extractDocument(parser.parse(is));
    } catch (Exception ex)
    {
      return null;
    }
  }

  public static Javadoc extractStream(InputStream is)
  {
    Parser<Document> parser = Html.newParser();
    return extractDocument(parser.parse(is));
  }

  public static Javadoc extractDocument(Document doc)
  {
    try
    {
      Javadoc documentation = new Javadoc();
      XPath xPath = XPathFactory.newInstance().newXPath();
      Node description = (Node) xPath.compile("//div[@class='description']").evaluate(doc, XPathConstants.NODE);
      if (description != null)
      {
        documentation.description = getString(description);
      }

      Node ctors = (Node) xPath.compile("//li/a[@name='constructor.detail']").evaluate(doc, XPathConstants.NODE);
      if (ctors != null)
      {
        documentation.ctors = new ArrayList<>();
        NodeList set = (NodeList) xPath.compile("./ul").evaluate(ctors.getParentNode(), XPathConstants.NODESET);
        for (int i = 0; i < set.getLength(); ++i)
        {
          documentation.ctors.add(getString(set.item(i)));
        }
      }

      Node methods = (Node) xPath.compile("//li/a[@name='method.detail']").evaluate(doc, XPathConstants.NODE);
      if (methods != null)
      {
        NodeList set = (NodeList) xPath.compile("./ul").evaluate(methods.getParentNode(), XPathConstants.NODESET);
        documentation.methods = convertNodes(set);
      }

      Node inner = (Node) xPath.compile("//li/a[@name='nested_class_summary']").evaluate(doc, XPathConstants.NODE);
      if (inner != null)
      {
        NodeList set = (NodeList) xPath.compile("./ul").evaluate(inner.getParentNode(), XPathConstants.NODESET);
        documentation.inner = convertNodes(set);
      }

      Node fields = (Node) xPath.compile("//li/a[@name='field.detail']").evaluate(doc, XPathConstants.NODE);
      if (fields != null)
      {
        NodeList set = (NodeList) xPath.compile("./ul").evaluate(methods.getParentNode(), XPathConstants.NODESET);
        documentation.fields = convertNodes(set);
      }

      return documentation;
    } catch (IOException | XPathExpressionException ex)
    {
      return null;
    }
  }

  private static String getString(Node node) throws IOException
  {
    try (ByteArrayOutputStream baos = new ByteArrayOutputStream())
    {
      HtmlWriter hw = new HtmlWriter(baos);
      hw.write(node);
      hw.close();
      return baos.toString();
    }
  }

  private static List<String> convertNodes(NodeList nl) throws IOException
  {
    List<String> out = new ArrayList<>();
    for (int i = 0; i < nl.getLength(); ++i)
    {
      out.add(getString(nl.item(i)));
    }
    return out;
  }
}
