package org.jpype.html;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import org.w3c.dom.Document;


/**
 *
 * @author nelson85
 */
public class Main
{

  public static void main(String[] args) throws IOException
  {
    //<editor-fold desc="elements" defaultstate="collapsed">
    try (InputStream is = ClassLoader.getSystemClassLoader().getResourceAsStream("com/google/gson/JsonArray.html"))
    {
      Parser<Document> parser = Html.newParser();
      Document doc = parser.parse(is);
      try (OutputStream os = Files.newOutputStream(Paths.get("tmp.html")))
      {
        HtmlWriter writer = new HtmlWriter(os);
        writer.writeDocument(doc);

      }
    }
  }
}
