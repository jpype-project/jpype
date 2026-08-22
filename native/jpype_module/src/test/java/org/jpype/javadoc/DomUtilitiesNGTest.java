// --- file: org/jpype/javadoc/DomUtilitiesNGTest.java ---
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
package org.jpype.javadoc;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Direct, focused test of the package-private {@link DomUtilities#transferContents}
 * helper, in isolation from the larger JavadocTransformer pipeline that
 * exercises it indirectly.
 */
public class DomUtilitiesNGTest
{

  @Test
  public void testTransferContentsMovesAllChildrenAndEmptiesSource() throws Exception
  {
    DocumentBuilder db = DocumentBuilderFactory.newInstance().newDocumentBuilder();
    Document doc = db.newDocument();

    Element source = doc.createElement("source");
    source.appendChild(doc.createTextNode("hello "));
    Element child = doc.createElement("child");
    child.appendChild(doc.createTextNode("world"));
    source.appendChild(child);

    Element dest = doc.createElement("dest");
    dest.appendChild(doc.createTextNode("existing-"));

    DomUtilities.transferContents(dest, source);

    assertFalse(source.hasChildNodes(), "source should be emptied");
    assertEquals(dest.getTextContent(), "existing-hello world");
    assertEquals(dest.getChildNodes().getLength(), 3);
  }

  @Test
  public void testTransferContentsOnEmptySourceIsNoOp() throws Exception
  {
    DocumentBuilder db = DocumentBuilderFactory.newInstance().newDocumentBuilder();
    Document doc = db.newDocument();
    Element source = doc.createElement("source");
    Element dest = doc.createElement("dest");
    dest.appendChild(doc.createTextNode("keep"));

    DomUtilities.transferContents(dest, source);

    assertFalse(source.hasChildNodes());
    assertEquals(dest.getTextContent(), "keep");
  }
}
