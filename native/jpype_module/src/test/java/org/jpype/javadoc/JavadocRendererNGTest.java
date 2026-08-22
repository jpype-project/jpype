// --- file: org/jpype/javadoc/JavadocRendererNGTest.java ---
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
import org.w3c.dom.Node;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Drives a hand-built post-transform DOM (the shape JavadocTransformer
 * normally produces) through the real {@link JavadocRenderer#render(Node)}
 * entry point, to exercise the section renderers that a plain successful
 * end-to-end run tends not to reach: block quotes, code blocks, headers,
 * ordered lists, parameter entries and throws entries.
 */
public class JavadocRendererNGTest
{

  private static Document newDocument() throws Exception
  {
    DocumentBuilder db = DocumentBuilderFactory.newInstance().newDocumentBuilder();
    return db.newDocument();
  }

  private static Element el(Document doc, String name, String text)
  {
    Element e = doc.createElement(name);
    if (text != null)
      e.appendChild(doc.createTextNode(text));
    return e;
  }

  /**
   * Builds:
   * <pre>
   * &lt;member&gt;
   *   &lt;title&gt;myMethod&lt;/title&gt;
   *   &lt;signature&gt;public void myMethod()&lt;/signature&gt;
   *   &lt;description&gt;
   *     &lt;p&gt;Intro text.&lt;/p&gt;
   *     &lt;blockquote&gt;&lt;p&gt;Quoted wisdom.&lt;/p&gt;&lt;/blockquote&gt;
   *     &lt;codeblock&gt;System.out.println("hi");&lt;/codeblock&gt;
   *     &lt;h2&gt;Header Text&lt;/h2&gt;
   *     &lt;ol&gt;&lt;li&gt;first item&lt;/li&gt;&lt;li&gt;second item&lt;/li&gt;&lt;/ol&gt;
   *   &lt;/description&gt;
   *   &lt;details&gt;
   *     &lt;parameters&gt;&lt;parameter name="x" type="int"&gt;the value&lt;/parameter&gt;&lt;/parameters&gt;
   *     &lt;throws&gt;&lt;exception name="IOException"&gt;if it fails&lt;/exception&gt;&lt;/throws&gt;
   *     &lt;returns&gt;the result&lt;/returns&gt;
   *   &lt;/details&gt;
   * &lt;/member&gt;
   * </pre>
   */
  @Test
  public void testRenderExercisesAllSectionRenderers() throws Exception
  {
    Document doc = newDocument();
    Element member = doc.createElement("member");

    member.appendChild(el(doc, "title", "myMethod"));
    member.appendChild(el(doc, "signature", "public void myMethod()"));

    Element description = doc.createElement("description");
    description.appendChild(el(doc, "p", "Intro text."));

    Element blockquote = doc.createElement("blockquote");
    blockquote.appendChild(el(doc, "p", "Quoted wisdom."));
    description.appendChild(blockquote);

    description.appendChild(el(doc, "codeblock", "System.out.println(\"hi\");"));
    description.appendChild(el(doc, "h2", "Header Text"));

    Element ol = doc.createElement("ol");
    Element li1 = doc.createElement("li");
    li1.appendChild(doc.createTextNode("first item"));
    Element li2 = doc.createElement("li");
    li2.appendChild(doc.createTextNode("second item"));
    ol.appendChild(li1);
    ol.appendChild(li2);
    description.appendChild(ol);

    member.appendChild(description);

    Element details = doc.createElement("details");

    Element parameters = doc.createElement("parameters");
    Element parameter = doc.createElement("parameter");
    parameter.setAttribute("name", "x");
    parameter.setAttribute("type", "int");
    parameter.appendChild(doc.createTextNode("the value"));
    parameters.appendChild(parameter);
    details.appendChild(parameters);

    Element throwsEl = doc.createElement("throws");
    Element exception = doc.createElement("exception");
    exception.setAttribute("name", "IOException");
    exception.appendChild(doc.createTextNode("if it fails"));
    throwsEl.appendChild(exception);
    details.appendChild(throwsEl);

    details.appendChild(el(doc, "returns", "the result"));

    member.appendChild(details);

    String out = new JavadocRenderer().render(member);

    // renderBlockQuote: indented paragraph text
    assertTrue(out.contains("Quoted wisdom."), out);

    // renderCodeBlock: rst code-block directive wrapping the code text
    assertTrue(out.contains(".. code-block: java"), out);
    assertTrue(out.contains("System.out.println(\"hi\");"), out);

    // renderHeader: text followed by a matching dashed underline
    assertTrue(out.contains("Header Text\n-----------"), out);

    // renderOrdered: numbered list items
    assertTrue(out.contains("1.  first item"), out);
    assertTrue(out.contains("2.  second item"), out);

    // renderParameter: "name (type): text"
    assertTrue(out.contains("x (int): the value"), out);

    // renderThrow: "name: text"
    assertTrue(out.contains("IOException: if it fails"), out);

    // renderDetails' "returns" branch via the SECTIONS map
    assertTrue(out.contains("Returns:"), out);
    assertTrue(out.contains("the result"), out);
  }
}
