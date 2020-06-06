package org.jpype.html;

public interface HtmlHandler
{

  void cdata(String text);

  void comment(String contents);

  void endDocument();

  void endElement(String name);

  void startDocument();

  void startElement(String name, String attr);

  void text(String text);

  public Object getResult();

  public void directive(String content);

}
