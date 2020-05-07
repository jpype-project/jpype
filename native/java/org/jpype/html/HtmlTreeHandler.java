package org.jpype.html;

import java.util.LinkedList;
import javax.xml.parsers.DocumentBuilder;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;


/**
 * HTML document handler which creates an HTML tree.
 */
public class HtmlTreeHandler implements HtmlHandler
{

  LinkedList<Element> elementStack = new LinkedList<>();
  DocumentBuilder db = null;
  Document root = db.newDocument();
  Node current = root;

  @Override
  public void startElement(String name, String attr)
  {
    Element elem = root.createElement(name);

    elem.setAttribute(name, name);
//    Attr attr = root.createAttribute(name);

    // FIXME handle attributes
    //new Element(name, attr);
    current.appendChild(elem);
    if (Html.VOID_ELEMENTS.contains(name))
      return;
    current = elem;
    elementStack.add(elem);
  }

  @Override
  public void endElement(String name)
  {
    if (elementStack.isEmpty())
      throw new RuntimeException("Empty stack");
    Element last = elementStack.removeLast();
    if (!last.getNodeName().equals(name))
      throw new RuntimeException("mismatch element " + name + " " + last.getNodeName());
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
