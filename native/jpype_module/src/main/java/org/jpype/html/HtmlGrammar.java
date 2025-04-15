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
import org.jpype.html.Parser.Entity;
import org.jpype.html.Parser.Rule;

public class HtmlGrammar implements Parser.Grammar
{

  final static HtmlGrammar INSTANCE = new HtmlGrammar();

  private HtmlGrammar()
  {
  }

  @Override
  public void start(Parser p)
  {
    p.state = State.FREE;
    ((HtmlParser) p).handler.startDocument();
  }

  @Override
  public Object end(Parser p)
  {
    ((HtmlParser) p).handler.endDocument();
    return ((HtmlParser) p).handler.getResult();
  }

  // BeginElement does < </ and <! via lookhead
  static StringBuilder promote(Entity e)
  {
    if (e.value != null && e.value instanceof StringBuilder)
      return (StringBuilder) e.value;
    StringBuilder sb = new StringBuilder(e.toString());
    e.value = sb;
    e.token = Token.TEXT;
    return (StringBuilder) sb;
  }

  static HtmlGrammar getGrammar(Parser p)
  {
    return ((HtmlGrammar) p.grammar);
  }

  static HtmlHandler getHandler(Parser p)
  {
    return ((HtmlParser) p).handler;
  }

  /**
   * Send everything out to as text.
   */
  void flushText(Parser<?> parser)
  {
    if (parser.stack.isEmpty())
      return;

    Entity last = parser.stack.removeLast();
    StringBuilder s = new StringBuilder();
    for (Entity e : parser.stack)
    {
      s.append(e.toString());
    }
    ((HtmlParser) parser).handler.text(s.toString());
    parser.stack.clear();
    parser.stack.add(last);
  }

//<editor-fold desc="state">
  enum State implements Parser.State
  {
    FREE(freeTokens, freeRules),
    ELEMENT(elementTokens, elementRules),
    DIRECTIVE(directiveTokens, directiveRules),
    CDATA(cdataTokens, cdataRules),
    COMMENT(commentTokens, commentRules);

    Token[] tokens;
    Rule[] rules;

    State(Token[] tokens, Rule[] rules)
    {
      this.tokens = tokens;
      this.rules = rules;
    }

    @Override
    public Token[] getTokens()
    {
      return this.tokens;
    }

    @Override
    public Rule[] getRules()
    {
      return this.rules;
    }
  }

//</editor-fold>
//<editor-fold desc="tokens" defaultstate="collapsed">
  enum Token implements Parser.Token
  {
    TEXT,
    BANG("!"),
    DASH("-"),
    LT("<"),
    GT(">"),
    SLASH("/"),
    AMP("&"),
    SEMI(";"),
    LSB("["),
    RSB("]"),
    CLOSE("</"),
    QUOTE("\""),
    SQUOTE("'"),
    DECL_DIRECTIVE("<!");

    byte value;
    String text;

    Token()
    {
    }

    Token(String s)
    {
      text = s;
      if (s.length() == 1)
        value = (byte) s.charAt(0);
    }

    @Override
    final public boolean matches(byte b)
    {
      if (value == 0)
        return true;
      return b == value;
    }

    @Override
    public boolean runs()
    {
      return this == Token.TEXT;
    }

    @Override
    public String toString()
    {
      if (text != null)
        return text;
      return "TEXT";
    }
  }
//</editor-fold>
//<editor-fold desc="rules">

  final static Rule escaped = new Escaped();
  final static Rule slash = new Cleanup();
  final static Rule mergeText = new MergeText();
  final static Rule beginElement = new BeginElement();
  final static Rule startElement = new StartElement();
  final static Rule completeElement = new CompleteElement();
  final static Rule endElement = new EndElement();
  final static Rule quote = new StartQuote();

  final static Token[] freeTokens = tokens(
          Token.LT, Token.AMP, Token.SEMI, Token.TEXT);
  final static Token[] elementTokens = tokens(
          Token.BANG, Token.AMP, Token.LT, Token.SEMI, Token.SLASH,
          Token.GT, Token.QUOTE, Token.SQUOTE, Token.TEXT);
  final static Token[] directiveTokens = tokens(
          Token.DASH, Token.LSB, Token.RSB, Token.LT, Token.GT, Token.QUOTE,
          Token.SQUOTE, Token.TEXT);
  final static Token[] cdataTokens = tokens(
          Token.LSB, Token.RSB, Token.GT, Token.TEXT);
  final static Token[] commentTokens = tokens(
          Token.DASH, Token.GT, Token.TEXT);
  final static Token[] quoteTokens = tokens(
          Token.QUOTE, Token.TEXT);
  final static Token[] squoteTokens = tokens(
          Token.SQUOTE, Token.TEXT);

  final static Rule[] freeRules = rules(
          beginElement, escaped, mergeText);
  final static Rule[] elementRules = rules(
          mergeText, quote, startElement, endElement, completeElement, escaped, slash);
  final static Rule[] directiveRules = rules(quote,
          new EndDirective(), mergeText);
  final static Rule[] cdataRules = rules(
          new EndCData());
  final static Rule[] commentRules = rules(
          new EndComment(), new StartComment());

  private static Rule[] rules(Rule... t)
  {
    return t;
  }

  private static Token[] tokens(Token... t)
  {
    return t;
  }

//</editor-fold>
//<editor-fold desc="inner" defaultstate="collapsed">
  private static class Cleanup implements Rule
  {

    @Override
    public boolean apply(Parser<?> parser, Entity entity)
    {
      if (entity.token == Token.SLASH)
      {
        parser.lookahead = this::next;
        return false;
      }

      if (entity.token == Token.GT && parser.stack.size() > 4)
      {
        // This it to help debug a rare problem
        for (Entity e : parser.stack)
        {
          System.out.print(e.token);
          System.out.print("(");
          System.out.print(e.value);
          System.out.print(") ");
        }
        System.out.println();
        throw new RuntimeException("Need cleanup");
      }
      return false;
    }

    private boolean next(Parser<?> parser, Entity entity)
    {
      if (entity.token != Token.GT)
      {
        parser.stack.removeLast();
        parser.stack.removeLast();
        // parser.stack.getLast().token = Token.TEXT;
        entity.value = "/" + entity.value;
        parser.stack.add(entity);
      }
      return false;
    }
  }

  private static class Escaped extends Parser.MatchRule
  {

    public Escaped()
    {
      super(Token.AMP, Token.TEXT, Token.SEMI);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      Entity e2 = stack.removeLast();
      Entity e1 = stack.removeLast();
      // Currently we do not verify the text contents
      Entity e0 = stack.getLast();
      promote(e0).append(e1.toString()).append(e2.toString());
    }
  }

  static class MergeText extends Parser.MatchRule
  {

    public MergeText()
    {
      super(Token.TEXT, Token.TEXT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      Entity t2 = stack.removeLast();
      Entity t1 = stack.getLast();
      promote(t1).append(t2.toString());
    }
  }

//</editor-fold>
//<editor-fold desc="elements" defaultstate="collapsed">
  static class BeginElement implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (entity.token != Token.LT)
        return false;
      getGrammar(parser).flushText(parser);
      parser.state = State.ELEMENT;
      parser.lookahead = this::next;
      return true;
    }

    public boolean next(Parser<?> parser, Parser.Entity entity)
    {
      if (entity.token == Token.SLASH)
      {
        parser.stack.removeLast();
        parser.stack.getLast().token = Token.CLOSE;
        return true;
      }

      if (entity.token == Token.BANG)
      {
        parser.stack.removeLast();
        Entity last = parser.stack.getLast();
        last.token = Token.DECL_DIRECTIVE;

        parser.lookahead = new Directive();
        parser.state = State.DIRECTIVE;
        return true;
      }
      return false;
    }
  }

  static class StartElement extends Parser.MatchRule
  {

    StartElement()
    {
      super(Token.LT, Token.TEXT, Token.GT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      stack.removeLast();
      Entity e1 = stack.removeLast();
      stack.removeLast();
      String content = e1.value.toString();
      getGrammar(parser).flushText(parser);
      String[] parts = content.split("\\s+", 2);
      if (parts.length == 1)
        getHandler(parser).startElement(content, null);
      else
        getHandler(parser).startElement(parts[0], parts[1]);
      parser.state = State.FREE;
    }

  }

  static class CompleteElement extends Parser.MatchRule
  {

    CompleteElement()
    {
      super(Token.LT, Token.TEXT, Token.SLASH, Token.GT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      stack.removeLast(); // >
      stack.removeLast(); // /
      Entity e1 = stack.removeLast();
      stack.removeLast(); // <
      String content = e1.value.toString();
      stack.clear();
      int i = content.indexOf(" ");

      if (i == -1)
      {
        getHandler(parser).startElement(content, null);
        getHandler(parser).endElement(content);
      } else
      {
        String name = content.substring(0, i);
        String attr = content.substring(i).trim();
        getHandler(parser).startElement(name, attr);
        getHandler(parser).endElement(name);
      }
      parser.state = State.FREE;
    }
  }

  static class EndElement extends Parser.MatchRule
  {

    EndElement()
    {
      super(Token.CLOSE, Token.TEXT, Token.GT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      Entity e2 = stack.removeLast();
      Entity e1 = stack.removeLast();
      Entity e0 = stack.removeLast();
      String content = e1.value.toString();
      getGrammar(parser).flushText(parser);
      getHandler(parser).endElement(content);
      parser.state = State.FREE;
    }
  }

  static class EndDirective extends Parser.MatchRule
  {

    EndDirective()
    {
      super(Token.DECL_DIRECTIVE, Token.TEXT, Token.GT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      Entity e2 = stack.removeLast();
      Entity e1 = stack.removeLast();
      Entity e0 = stack.removeLast();
      String content = e1.value.toString();
      getGrammar(parser).flushText(parser);
      getHandler(parser).directive(content);
      parser.state = State.FREE;
    }
  }

//</editor-fold>
//<editor-fold desc="comment" defaultstate="collapsed">
  // This is a look ahead rule
  static class Directive implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (entity.token == Token.LSB)
      {
        parser.lookahead = new CData();
        return true;
      }

      if (entity.token == Token.DASH)
      {
        parser.lookahead = new Comment();
        return true;
      }

      return false;
    }

  }

  static class CData implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (entity.token != Token.TEXT)
        parser.error("Expected CDATA");
      parser.lookahead = this::next;
      return true;
    }

    public boolean next(Parser parser, Parser.Entity entity)
    {
      if (entity.token != Token.LSB)
        parser.error("Expected [");
      parser.stack.clear();
      parser.state = State.CDATA;
      return true;
    }

  }

  static class EndCData extends Parser.MatchRule
  {

    public EndCData()
    {
      super(Token.RSB, Token.RSB, Token.GT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      stack.removeLast(); // >
      stack.removeLast(); // ]
      stack.removeLast(); // ]
      Entity first = stack.removeFirst();
      StringBuilder sb = promote(first);
      for (Entity e : stack)
      {
        sb.append(e.toString());
      }
      stack.clear();
      ((HtmlParser) parser).handler.cdata(sb.toString());
      parser.state = State.FREE;
    }
  }

  static class Comment implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (entity.token != Token.DASH)
        parser.error("Expected -");
      parser.lookahead = this::next;
      parser.stack.clear();
      parser.state = State.COMMENT;
      return true;
    }

    public boolean next(Parser parser, Parser.Entity entity)
    {
      if (entity.token == Token.DASH)
        parser.error("Bad comment(-)");
      if (entity.token == Token.GT)
        parser.error("Bad comment(>)");
      return false;
    }
  }

  static class StartComment extends Parser.MatchRule
  {

    public StartComment()
    {
      super(Token.LT, Token.BANG, Token.DASH, Token.DASH);
    }

    @Override
    public void execute(Parser parser)
    {
      parser.lookahead = this::next;
    }

    public boolean next(Parser parser, Parser.Entity entity)
    {
      if (entity.token == Token.GT)
        return false;
      parser.error("Comment contains <!--");
      return true;
    }
  }

  /**
   * Double dash in Comments.
   */
  static class EndComment extends Parser.MatchRule
  {

    public EndComment()
    {
      super(Token.DASH, Token.DASH, Token.GT);
    }

    @Override
    public void execute(Parser parser)
    {
      LinkedList<Entity> stack = parser.stack;
      stack.removeLast(); // >
      stack.removeLast(); // -
      stack.removeLast(); // -
      if (stack.isEmpty())
        return;
      Entity first = stack.removeFirst();
      StringBuilder sb = promote(first);
      for (Entity e : stack)
      {
        sb.append(e.toString());
      }
      stack.clear();
      ((HtmlParser) parser).handler.comment(sb.toString());
      parser.state = State.FREE;
    }
  }

//</editor-fold>
//<editor-fold desc="quote">
  static class StartQuote implements Rule
  {

    @Override
    public boolean apply(Parser parser, Parser.Entity entity)
    {
      if (entity.token == Token.QUOTE
              || entity.token == Token.SQUOTE)
      {
        parser.state = new Quoted(entity.token, parser.state);
        parser.stack.removeLast();
        parser.add(Token.TEXT, entity.toString());
        return true;
      }
      return false;
    }
  }

  static class Quoted implements Parser.State, Parser.Rule
  {

    Parser.Token[] tokens;
    Parser.Rule[] rules;
    private Parser.State state;

    Quoted(Parser.Token token, Parser.State state)
    {
      this.tokens = new Parser.Token[]
      {
        token, Token.TEXT
      };
      rules = new Parser.Rule[]
      {
        this, mergeText
      };
      this.state = state;
    }

    @Override
    public Parser.Token[] getTokens()
    {
      return tokens;
    }

    @Override
    public Parser.Rule[] getRules()
    {
      return rules;
    }

    @Override
    public boolean apply(Parser<?> parser, Parser.Entity entity)
    {
      if (entity.token != tokens[0])
        return false;
      parser.stack.removeLast();
      Entity last = parser.stack.getLast();
      promote(last).append('"');
      parser.state = state;
      return true;
    }
  }

//</editor-fold>
}
