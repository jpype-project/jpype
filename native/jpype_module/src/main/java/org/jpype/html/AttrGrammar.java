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

import org.jpype.html.Parser.Entity;
import org.jpype.html.Parser.Rule;
import org.w3c.dom.Attr;

public class AttrGrammar implements Parser.Grammar
{

  static final AttrGrammar INSTANCE = new AttrGrammar();

  private AttrGrammar()
  {
  }

  @Override
  public void start(Parser p)
  {
    p.state = State.FREE;
    ((AttrParser) p).attrs.clear();
  }

  @Override
  public Object end(Parser p)
  {
    return ((AttrParser) p).attrs;
  }

//<editor-fold desc="tokens">
  enum Token implements Parser.Token
  {
    TEXT,
    QUOTE("\""),
    SQUOTE("'"),
    EQ("="),
    WHITESPACE(" ");

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
      if (value == ' ')
        return Character.isWhitespace(b);
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

  final static Token[] freeTokens = tokens(
          Token.QUOTE, Token.SQUOTE, Token.EQ, Token.WHITESPACE, Token.TEXT);
  final static Token[] qtTokens = tokens(
          Token.QUOTE, Token.TEXT);
  final static Token[] sqtTokens = tokens(
          Token.SQUOTE, Token.TEXT);

  final static Rule ignoreWS = new IgnoreWSRule();
  final static Rule quoteRule = new QuoteRule();
  final static Rule endRule = new EndQuoteRule();
  final static Rule attrRule = new AttrRule();
  final static Rule boolRule = new BooleanRule();

  final static Rule[] freeRules = rules(attrRule, boolRule, ignoreWS, quoteRule);
  final static Rule[] qtRules = rules(endRule);

  static Token[] tokens(Token... t)
  {
    return t;
  }

  static Rule[] rules(Rule... t)
  {
    return t;
  }
//</editor-fold>
//<editor-fold desc="state">

  enum State implements Parser.State
  {
    FREE(freeTokens, freeRules),
    IN_QUOTE(qtTokens, qtRules),
    IN_SQUOTE(sqtTokens, qtRules);

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
//<editor-fold desc="rules">
  static class IgnoreWSRule implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (entity.token != Token.WHITESPACE)
        return false;
      parser.stack.removeLast();
      return true;
    }
  }

  static class QuoteRule implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (entity.token == Token.QUOTE)
      {
        parser.state = State.IN_QUOTE;
        parser.stack.removeLast();
        return true;
      }
      if (entity.token == Token.SQUOTE)
      {
        parser.state = State.IN_SQUOTE;
        parser.stack.removeLast();
        return true;
      }
      return false;
    }
  }

  static class EndQuoteRule implements Rule
  {

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      if (State.IN_QUOTE == parser.state && entity.token == Token.QUOTE)
      {
        parser.state = State.FREE;
        parser.stack.removeLast();
        return true;
      }
      if (State.IN_SQUOTE == parser.state && entity.token == Token.SQUOTE)
      {
        parser.state = State.FREE;
        parser.stack.removeLast();
        return true;
      }
      return false;
    }
  }

  static class AttrRule extends Parser.MatchRule
  {

    AttrRule()
    {
      super(Token.TEXT, Token.EQ, Token.TEXT);
    }

    @Override
    public void execute(Parser parser)
    {
      Entity e2 = (Entity) parser.stack.removeLast();
      Entity e1 = (Entity) parser.stack.removeLast();
      Entity e0 = (Entity) parser.stack.removeLast();
      AttrParser aparser = (AttrParser) parser;
      Attr attr = aparser.doc.createAttribute((String) e0.value);
      attr.setNodeValue((String) e2.value);
      aparser.attrs.add(attr);
    }
  }

  static class BooleanRule extends Parser.MatchRule
  {

    BooleanRule()
    {
      super(Token.TEXT, Token.TEXT);
    }

    @Override
    public void execute(Parser parser)
    {
      Entity e2 = (Entity) parser.stack.removeLast();
      Entity e1 = (Entity) parser.stack.removeLast();
      AttrParser aparser = (AttrParser) parser;
      Attr attr = aparser.doc.createAttribute((String) e1.value);
      attr.setNodeValue((String) e1.value);
      aparser.attrs.add(attr);
      parser.stack.add(e2);
    }
  }
//</editor-fold>
}
