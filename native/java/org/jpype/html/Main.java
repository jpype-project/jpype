package org.jpype.html;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.w3c.dom.Document;

public class Main
{

  public static void main(String[] args) throws IOException
  {
    //<editor-fold desc="elements" defaultstate="collapsed">
//    try (InputStream is = ClassLoader.getSystemClassLoader().getResourceAsStream("com/google/gson/JsonArray.html"))
    try (InputStream is = Files.newInputStream(Paths.get("in.html")))
    {
      Parser<Document> parser = Html.newParser();
      Document doc = parser.parse(is);
      XPath xPath = XPathFactory.newInstance().newXPath();
      System.out.println(xPath.compile("//div[@class='description']").evaluate(doc, XPathConstants.NODE));
      System.out.println(xPath.compile("//li/a[@name='constructor.detail']").evaluate(doc, XPathConstants.NODE));
      System.out.println(xPath.compile("//li/a[@name='method.detail']").evaluate(doc, XPathConstants.NODE));
      System.out.println(xPath.compile("//li/a[@name='nested_class_summary']").evaluate(doc, XPathConstants.NODE));
      System.out.println(xPath.compile("//li/a[@name='field_summary']").evaluate(doc, XPathConstants.NODE));
      try (OutputStream os = Files.newOutputStream(Paths.get("tmp.html")))
      {
        HtmlWriter writer = new HtmlWriter(os);
        writer.writeDocument(doc);
      }
    } catch (XPathExpressionException ex)
    {
      throw new RuntimeException(ex);
    }
  }
}
