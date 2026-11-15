// --- file: org/jpype/html/HtmlGrammarNGTest.java ---
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

import java.util.List;
import org.jpype.html.Parser.Entity;
import org.w3c.dom.Attr;
import org.w3c.dom.CDATASection;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Drives real HTML content through {@link Html#newParser()} to exercise the
 * grammar-internal state machine pieces of {@link HtmlGrammar}, and directly
 * exercises the package-private nested rule classes that are not reachable
 * (or only very awkwardly reachable) via the character-stream entry point.
 *
 * This is placed in package org.jpype.html deliberately, matching that
 * package's own nested (package-private) grammar classes, so that the
 * package-private constructors/methods on those classes can be called
 * directly where the top-level parse API cannot exercise them.
 */
public class HtmlGrammarNGTest
{

  //<editor-fold desc="HtmlGrammar$CompleteElement">
  @Test
  public void testCompleteElementNoAttributes()
  {
    Document doc = Html.newParser().parse("<wrap><foo/></wrap>");
    Element wrap = (Element) doc.getFirstChild();
    Element foo = (Element) wrap.getFirstChild();
    assertEquals(foo.getTagName(), "foo");
    assertFalse(foo.hasChildNodes());
  }

  @Test
  public void testCompleteElementWithAttributes()
  {
    Document doc = Html.newParser().parse("<wrap><foo bar=\"baz\"/></wrap>");
    Element wrap = (Element) doc.getFirstChild();
    Element foo = (Element) wrap.getFirstChild();
    assertEquals(foo.getTagName(), "foo");
    assertEquals(foo.getAttribute("bar"), "baz");
  }
  //</editor-fold>

  //<editor-fold desc="HtmlGrammar$CData / $EndCData">
  @Test
  public void testCDataSection()
  {
    Document doc = Html.newParser().parse("<wrap><![CDATA[some <data> & stuff]]></wrap>");
    Element wrap = (Element) doc.getFirstChild();
    Node cdata = wrap.getFirstChild();
    assertEquals(cdata.getNodeType(), Node.CDATA_SECTION_NODE);
    assertEquals(((CDATASection) cdata).getData(), "some <data> & stuff");
  }
  //</editor-fold>

  //<editor-fold desc="HtmlGrammar$Cleanup (private - only reachable via char stream)">
  @Test
  public void testCleanupHandlesSlashInUnquotedAttributeValue()
  {
    // An unquoted attribute value containing a slash that is not
    // immediately followed by '>' takes the ELEMENT state's "slash" rule
    // (HtmlGrammar$Cleanup): SLASH is provisionally matched as a possible
    // self-close, then un-done and merged back into the running text once
    // the following token turns out not to be '>'. This is the normal
    // real-world case of an unquoted URL attribute value.
    Document doc = Html.newParser().parse("<a href=/index.html>Link</a>");
    Element a = (Element) doc.getFirstChild();
    assertEquals(a.getTagName(), "a");
    assertEquals(a.getAttribute("href"), "/index.html");
    assertEquals(a.getTextContent(), "Link");
  }

  // Note: HtmlGrammar$Cleanup also has a branch that throws
  // RuntimeException("Need cleanup") when a GT token arrives with more than
  // 4 entities still on the element stack. That branch is a debug
  // last-resort assertion for a state the authors describe as "a rare
  // problem" and is not reachable through any legal HTML input constructed
  // from the grammar's own rules (by the time GT can be seen, the
  // preceding rules always reduce the stack back down first); Cleanup's
  // class and constructor are also `private`, so it cannot be
  // instantiated directly even from this same-package test. Left
  // untested; see class javadoc comment on Cleanup for context.
  //</editor-fold>

  //<editor-fold desc="HtmlGrammar$StartComment (unreachable via char stream)">
  @Test
  public void testStartCommentDirectApplyAndImmediateClose()
  {
    // While parsing normal comment content, '<' and '!' are never tokenized
    // as LT/BANG (COMMENT state's token set is only DASH, GT, TEXT), and
    // Comment.apply() clears the stack when entering COMMENT state - so the
    // literal LT,BANG,DASH,DASH pattern StartComment matches against can
    // never actually appear on the stack via the character-stream parser.
    // We exercise it directly instead, which the package-private
    // constructor/methods on this same-package nested class allow.
    HtmlParser p = new HtmlParser();
    p.state = HtmlGrammar.State.COMMENT;
    p.add(HtmlGrammar.Token.LT, "<");
    p.add(HtmlGrammar.Token.BANG, "!");
    p.add(HtmlGrammar.Token.DASH, "-");
    Entity last = p.add(HtmlGrammar.Token.DASH, "-");

    HtmlGrammar.StartComment rule = new HtmlGrammar.StartComment();
    assertTrue(rule.apply(p, last), "StartComment should match a literal <!-- on the stack");

    // Immediately followed by '>' - no error, comment is allowed to close.
    Entity gt = p.add(HtmlGrammar.Token.GT, ">");
    assertFalse(rule.next(p, gt));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testStartCommentDirectNextErrorsOnNonGt()
  {
    HtmlParser p = new HtmlParser();
    p.state = HtmlGrammar.State.COMMENT;
    p.add(HtmlGrammar.Token.LT, "<");
    p.add(HtmlGrammar.Token.BANG, "!");
    p.add(HtmlGrammar.Token.DASH, "-");
    Entity last = p.add(HtmlGrammar.Token.DASH, "-");

    HtmlGrammar.StartComment rule = new HtmlGrammar.StartComment();
    rule.apply(p, last);

    // Anything other than '>' following a nested "<!--" is an error:
    // "Comment contains <!--"
    Entity text = p.add(HtmlGrammar.Token.TEXT, "x");
    rule.next(p, text);
  }
  //</editor-fold>

  //<editor-fold desc="Parser$Entity">
  @Test
  public void testEntityToStringFallsBackToTokenWhenValueIsNull()
  {
    HtmlParser p = new HtmlParser();
    Entity e = p.add(HtmlGrammar.Token.GT, null);
    assertNull(e.value);
    assertEquals(e.toString(), ">");
  }

  @Test
  public void testEntityToStringUsesValueWhenPresent()
  {
    HtmlParser p = new HtmlParser();
    Entity e = p.add(HtmlGrammar.Token.TEXT, "hello");
    assertEquals(e.toString(), "hello");
  }
  //</editor-fold>

  //<editor-fold desc="HtmlTreeHandler">
  @Test
  public void testCdataAppendsCDataSectionToCurrentNode()
  {
    HtmlTreeHandler handler = new HtmlTreeHandler();
    handler.startDocument();
    handler.startElement("div", null);
    handler.cdata("raw & stuff");
    Node root = (Node) handler.getResult();
    Node divElem = root.getFirstChild();
    Node cdata = divElem.getFirstChild();
    assertEquals(cdata.getNodeType(), Node.CDATA_SECTION_NODE);
    assertEquals(cdata.getNodeValue(), "raw & stuff");
  }

  @Test
  public void testGetPathReflectsOpenElementStackAndAttributes()
  {
    HtmlTreeHandler handler = new HtmlTreeHandler();
    handler.startDocument();
    assertEquals(handler.getPath(), "");
    handler.startElement("div", "class=\"outer\"");
    handler.startElement("span", null);
    String path = handler.getPath();
    assertTrue(path.startsWith("/div[class=outer"), path);
    assertTrue(path.endsWith("/span"), path);
  }
  //</editor-fold>

  //<editor-fold desc="AttrGrammar$Token">
  @Test
  public void testAttrGrammarTokenWhitespaceMatchesAnyWhitespaceByte()
  {
    assertTrue(AttrGrammar.Token.WHITESPACE.matches((byte) ' '));
    assertTrue(AttrGrammar.Token.WHITESPACE.matches((byte) '\t'));
    assertFalse(AttrGrammar.Token.WHITESPACE.matches((byte) 'x'));
  }

  @Test
  public void testAttrGrammarTokenTextMatchesAnyByte()
  {
    assertTrue(AttrGrammar.Token.TEXT.matches((byte) 'z'));
    assertTrue(AttrGrammar.Token.TEXT.runs());
    assertFalse(AttrGrammar.Token.EQ.runs());
  }

  @Test
  public void testParseAttributesQuotedEqualsAndBooleanAttribute()
  {
    Document doc = Html.newParser().parse("<x/>");
    List<Attr> attrs = Html.parseAttributes(doc, "class=\"foo bar\" disabled data-x='sq'");
    assertEquals(attrs.size(), 3);
    assertEquals(attrs.get(0).getName(), "class");
    assertEquals(attrs.get(0).getValue(), "foo bar");
    // boolean attribute: name used as its own value
    assertEquals(attrs.get(1).getName(), "disabled");
    assertEquals(attrs.get(1).getValue(), "disabled");
    // single-quoted value
    assertEquals(attrs.get(2).getName(), "data-x");
    assertEquals(attrs.get(2).getValue(), "sq");
  }
  //</editor-fold>
}
