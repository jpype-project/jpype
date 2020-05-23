package org.jpype.html;

import java.util.ArrayList;
import java.util.List;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;

public class AttrParser extends Parser<List<Attr>>
{
  final Document doc;
  final List<Attr> attrs = new ArrayList<>();

  public AttrParser(Document doc)
  {
    super(AttrGrammar.INSTANCE);
    this.doc = doc;
  }
}
