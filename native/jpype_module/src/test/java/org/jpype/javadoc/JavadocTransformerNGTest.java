// --- file: org/jpype/javadoc/JavadocTransformerNGTest.java ---
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

import org.jpype.html.Html;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Drives a small hand-written javadoc member fragment (shaped like what
 * JavadocExtractor pulls out of a real javadoc HTML page) through
 * {@link Html#newParser()} and then {@link JavadocTransformer#transformMember}
 * to exercise {@link JavadocTransformer#handleDetails} - the "dt"/"dd"
 * pairing logic that turns a &lt;dl&gt; of Parameters/Throws/Since entries
 * into &lt;parameters&gt;/&lt;throws&gt;/&lt;since&gt; structured elements.
 */
public class JavadocTransformerNGTest
{

  /**
   * Parses the fragment (wrapped in a single &lt;li&gt; container, matching
   * what JavadocExtractor hands to transformMember) and returns that &lt;li&gt;
   * node.
   */
  private static Node parseMember(String innerHtml)
  {
    Document doc = Html.newParser().parse("<li>" + innerHtml + "</li>");
    return doc.getFirstChild();
  }

  @Test
  public void testHandleDetailsParametersThrowsAndFallbackSection()
  {
    String html = "<h3>myMethod</h3>"
            + "<pre>public void myMethod(int&nbsp;x)</pre>"
            + "<div>Some description.</div>"
            + "<dl>"
            + "<dt>Parameters:</dt>"
            + "<dd><code>x</code> - the value</dd>"
            + "<dt>Throws:</dt>"
            + "<dd><code>IOException</code> - if it fails</dd>"
            + "<dt>Since:</dt>"
            + "<dd>1.0</dd>"
            + "</dl>";

    Node member = parseMember(html);
    Node result = new JavadocTransformer().transformMember(String.class, member);

    Element root = (Element) result;

    // title (h3 -> title) renamed
    NodeList titles = root.getElementsByTagName("title");
    assertEquals(titles.getLength(), 1);
    assertEquals(titles.item(0).getTextContent(), "myMethod");

    // "dl" renamed to "details"; dt "Parameters:"/"Throws:" recognized via
    // DETAIL_SECTIONS, and dd contents transferred into typed child
    // elements ("parameter"/"exception") - the ws.key.equals("parameters")
    // and ws.key.equals("throws") branches of handleDetails.
    NodeList parameters = root.getElementsByTagName("parameters");
    assertEquals(parameters.getLength(), 1);
    NodeList parameter = ((Element) parameters.item(0)).getElementsByTagName("parameter");
    assertEquals(parameter.getLength(), 1);
    Element param = (Element) parameter.item(0);
    assertEquals(param.getAttribute("name"), "x");
    assertEquals(param.getAttribute("type"), "int");
    assertTrue(param.getTextContent().contains("the value"), param.getTextContent());

    NodeList throwsSections = root.getElementsByTagName("throws");
    assertEquals(throwsSections.getLength(), 1);
    NodeList exception = ((Element) throwsSections.item(0)).getElementsByTagName("exception");
    assertEquals(exception.getLength(), 1);
    Element exc = (Element) exception.item(0);
    assertEquals(exc.getAttribute("name"), "IOException");
    assertTrue(exc.getTextContent().contains("if it fails"), exc.getTextContent());

    // "Since:" dt is a known DETAIL_SECTIONS key but its dd content takes
    // handleDetails' generic fallback branch (transferContents into the
    // renamed <since> section rather than a special typed child element).
    NodeList since = root.getElementsByTagName("since");
    assertEquals(since.getLength(), 1);
    assertTrue(since.item(0).getTextContent().contains("1.0"), since.item(0).getTextContent());
  }

  @Test
  public void testHandleDetailsUnknownAndSeePrefixedKeys()
  {
    // "Bogus:" hits handleDetails' final `else` branch (unrecognized key,
    // just logged); "See Also:" is an exact DETAIL_SECTIONS entry, and a
    // key that merely *starts with* "See " but isn't an exact match (e.g.
    // "See The Java Language Specification:") hits the jls fallback branch.
    String html = "<h3>otherMethod</h3>"
            + "<pre>public void otherMethod()</pre>"
            + "<div>Desc.</div>"
            + "<dl>"
            + "<dt>Bogus:</dt>"
            + "<dd>unused</dd>"
            + "<dt>See The Java Language Specification:</dt>"
            + "<dd>15.28</dd>"
            + "</dl>";

    Node member = parseMember(html);
    // Should not throw despite the unrecognized "Bogus:" key.
    Node result = new JavadocTransformer().transformMember(String.class, member);
    Element root = (Element) result;

    NodeList jls = root.getElementsByTagName("jls");
    assertEquals(jls.getLength(), 1);
    assertTrue(jls.item(0).getTextContent().contains("15.28"), jls.item(0).getTextContent());
  }
}
