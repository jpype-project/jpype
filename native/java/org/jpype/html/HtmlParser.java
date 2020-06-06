package org.jpype.html;

import org.w3c.dom.Document;

public class HtmlParser extends Parser<Document>
{
  HtmlHandler handler = new HtmlTreeHandler();

  public HtmlParser()
  {
    super(HtmlGrammar.INSTANCE);
  }

}
