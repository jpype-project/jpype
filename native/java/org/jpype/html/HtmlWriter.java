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
package org.jpype.html;

import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import org.w3c.dom.Attr;
import org.w3c.dom.CDATASection;
import org.w3c.dom.Comment;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.ProcessingInstruction;
import org.w3c.dom.Text;

public class HtmlWriter implements Closeable
{

  OutputStream os;
  BufferedWriter writer;

  public HtmlWriter(OutputStream os)
  {
    this.os = os;
    this.writer = new BufferedWriter(new OutputStreamWriter(os));
  }

  public static String asString(Node node) throws IOException
  {
    try (ByteArrayOutputStream baos = new ByteArrayOutputStream())
    {
      HtmlWriter hw = new HtmlWriter(baos);
      hw.write(node);
      hw.close();
      return baos.toString();
    }
  }

  public void write(Node n) throws IOException
  {

    if (n == null)
    {
      writer.write("NULL");
      return;
    }
    switch (n.getNodeType())
    {
      case Node.PROCESSING_INSTRUCTION_NODE:
        writeDirective((ProcessingInstruction) n);
        break;
      case Node.ELEMENT_NODE:
        writeElement((Element) n);
        break;
      case Node.DOCUMENT_FRAGMENT_NODE:
        writeChildren(n);
        break;
      case Node.TEXT_NODE:
        writeText((Text) n);
        break;
      case Node.COMMENT_NODE:
        writeComment((Comment) n);
        break;
      case Node.CDATA_SECTION_NODE:
        writeCData((CDATASection) n);
        break;
      default:
        throw new RuntimeException("unhandled " + n.getClass());
    }
  }

  public void writeChildren(Node doc) throws IOException
  {
    NodeList children = doc.getChildNodes();
    for (int i = 0; i < children.getLength(); ++i)
    {
      write(children.item(i));
    }
  }

  public void writeDirective(ProcessingInstruction d) throws IOException
  {
    writer.write("<!");
    writer.write(d.getTarget());
    writer.write(" ");
    writer.write(d.getData());
    writer.write(">");
  }

  public void writeElement(Element e) throws IOException
  {
    String name = e.getTagName();
    writer.write("<");
    writer.write(name);
    NamedNodeMap attributes = e.getAttributes();
    if (attributes.getLength() > 0)
    {
      for (int i = 0; i < attributes.getLength(); ++i)
      {
        writer.write(" ");
        Attr attr = (Attr) attributes.item(i);
        writer.write(attr.getName());
        writer.write("=\"");
        writer.write(attr.getValue());
        writer.write('"');
      }
    }
    if (Html.VOID_ELEMENTS.contains(name))
    {
      writer.write(">");
      return;
    }

    NodeList children = e.getChildNodes();

    if (children.getLength() == 0)
    {
      writer.write("/>");
    } else
    {
      writer.write(">");
      writeChildren(e);
      writer.write("</");
      writer.write(name);
      writer.write(">");
    }
  }

  private void writeComment(Comment comment) throws IOException
  {
    writer.write("<!--");
    writer.write(comment.getData());
    writer.write("-->");
  }

  private void writeCData(CDATASection cData) throws IOException
  {
    writer.write("<![CDATA[");
    writer.write(cData.getData());
    writer.write("]]>");
  }

  private void writeText(Text text) throws IOException
  {
    writer.write(text.getData());
  }

  @Override
  public void close() throws IOException
  {
    writer.close();
  }
}
