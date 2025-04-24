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

import java.util.LinkedList;
import java.util.ListIterator;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;

/**
 * HTML document handler which creates an HTML tree.
 */
public class HtmlTreeHandler implements HtmlHandler
{

  final Document root;
  LinkedList<Element> elementStack = new LinkedList<>();
  AttrParser attrParser;
  Node current;
  int errors = 0;

  public HtmlTreeHandler()
  {
    try
    {
      DocumentBuilder db = DocumentBuilderFactory.newInstance().newDocumentBuilder();
      root = db.newDocument();
      current = root;
      attrParser = new AttrParser(root);
    } catch (ParserConfigurationException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  private String lastNodeName()
  {
    if (this.elementStack.isEmpty())
      return "";
    return this.elementStack.getLast().getNodeName();
  }

  @Override
  public void startElement(String name, String attr)
  {
    name = name.toLowerCase().trim();
    String attr0 = attr;

    // Html has irregular end rules.
    while (Html.OPTIONAL_ELEMENTS.contains(name))
    {
      String close = lastNodeName() + ":" + name;
      if (Html.OPTIONAL_CLOSE.contains(close))
      {
//        System.out.print("AUTO ");
        this.endElement(lastNodeName());
      } else
        break;
    }

//    System.out.println(this.elementStack.size() + " " + name + " : " + attr);
    Element elem;
    try
    {
      elem = root.createElement(name);
    } catch (Exception ex)
    {
      throw new RuntimeException("Fail to create node '" + name + "'", ex);
    }
    if (attr != null)
    {
      for (Attr a : attrParser.parse(attr))
        elem.setAttributeNode(a);
    }
    current.appendChild(elem);
    if (Html.VOID_ELEMENTS.contains(name))
      return;
    current = elem;
    elementStack.add(elem);
  }

  public String getPath()
  {
    StringBuilder path = new StringBuilder();
    for (Element s : this.elementStack)
    {
      path.append("/");
      path.append(s.getNodeName());
      NamedNodeMap attrs = s.getAttributes();
      if (attrs.getLength() > 0)
      {
        path.append('[');
        for (int i = 0; i < attrs.getLength(); ++i)
        {
          Attr item = (Attr) attrs.item(i);
          path.append(item.getName());
          path.append('=');
          path.append(item.getValue());
          path.append(' ');
        }
        path.append(']');
      }
    }
    return path.toString();
  }

  @Override
  public void endElement(String name)
  {
    name = name.toLowerCase().trim();
    if (elementStack.isEmpty())
      throw new RuntimeException("Empty stack");
    Element last = elementStack.getLast();
    // Handle auto class tags
    while (!last.getNodeName().equals(name) && Html.OPTIONAL_ELEMENTS.contains(last.getNodeName()))
    {
//      System.out.print("AUTO2 ");
      endElement(last.getNodeName());
      last = elementStack.getLast();
    }
//    System.out.println(this.elementStack.size() - 1 + " ~" + name);
    if (!last.getNodeName().equals(name))
    {
      errors++;

      // Try to deal with unclosed tags gracefully.
      ListIterator<Element> iter = this.elementStack.listIterator(this.elementStack.size() - 1);
      int i = 0;
      while (iter.hasNext())
      {
        Element prev = iter.previous();
        if (prev.getNodeName().equals(name))
          break;
        i++;
      }
      if (iter.hasPrevious())
      {
        for (int j = 0; j < i; j++)
        {
          System.err.println("Ignoring missing close tag " + last.getNodeName() + ", got " + name + " at " + getPath());
          this.endElement(last.getNodeName());
        }
      } else if (errors > 3)
        throw new RuntimeException("mismatch element " + name
                + " " + last.getNodeName() + " at " + getPath());
      else
      {
        System.err.println("Ignoring mismatched element " + name
                + " " + last.getNodeName() + " at " + getPath());
        return;
      }
    }
    elementStack.removeLast();
    if (elementStack.isEmpty())
      current = root;
    else
      current = elementStack.getLast();
  }

  @Override
  public void comment(String contents)
  {
    if (contents.equals(">"))
      throw new RuntimeException();
    current.appendChild(root.createComment(contents));
  }

  @Override
  public void text(String text)
  {
//    System.out.println("  TEXT " + text);
    if (text.length() == 0)
      return;
    if (text.contains("<"))
      throw new RuntimeException("bad text `" + text + "` at " + getPath());
    if (current == root)
      return;
    current.appendChild(root.createTextNode(text));
  }

  @Override
  public void cdata(String text)
  {
    current.appendChild(root.createCDATASection(text));
  }

  @Override
  public void startDocument()
  {
  }

  @Override
  public void endDocument()
  {
  }

  @Override
  public Object getResult()
  {
    return root;
  }

  @Override
  public void directive(String content)
  {
    int i = content.indexOf(" ");
    current.appendChild(root.createProcessingInstruction(content.substring(0, i),
            content.substring(i).trim()));
  }

}
