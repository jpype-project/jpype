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

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.channels.ReadableByteChannel;
import java.util.LinkedList;
import java.util.ListIterator;

/**
 * Generic document parser.
 *
 * @param <T>
 */
public class Parser<T>
{

  final Grammar grammar;
  public State state = null;
  public Token last = null;
  public Rule lookahead = null;
  public LinkedList<Entity> stack = new LinkedList<>();

  Parser(Grammar grammar)
  {
    this.grammar = grammar;
  }

  public T parse(InputStream is)
  {
    ByteBuffer incoming = ByteBuffer.allocate(1024);
    ByteBuffer outgoing = ByteBuffer.allocate(1024);
    ReadableByteChannel channel = Channels.newChannel(is);
    stack.clear();
    grammar.start(this);
    try
    {
      while (true)
      {
        incoming.position(0);
        int rc = channel.read(incoming);
        if (rc < 0)
          break;
        int p = incoming.position();
        incoming.rewind();
        process(incoming, outgoing, rc);
      }
      flushTokens(outgoing);
    } catch (IOException ex)
    {
      throw new RuntimeException(ex);
    }
    return (T) grammar.end(this);
  }

  public T parse(String str)
  {
    byte[] b = str.getBytes();
    ByteBuffer incoming = ByteBuffer.wrap(b);
    ByteBuffer outgoing = ByteBuffer.allocate(1024);
    stack.clear();
    grammar.start(this);
    process(incoming, outgoing, b.length);
    flushTokens(outgoing);
    return (T) grammar.end(this);
  }

  private void process(ByteBuffer incoming, ByteBuffer outgoing, int rc)
  {
    while (incoming.position() < rc)
    {
      byte b = incoming.get();
      Token match = null;
      for (Token t : state.getTokens())
      {
        if (t.matches(b))
        {
          match = t;
          break;
        }
      }
      if (match == null)
        this.error("Unable to parse " + (char) b);
      else if (match.runs())
      {
        if (last != match)
          flushTokens(outgoing);
        if (!outgoing.hasRemaining())
          flushTokens(outgoing);
        outgoing.put(b);
      } else
      {
        if (outgoing.position() > 0)
          flushTokens(outgoing);
        processToken(match, null);
      }
      last = match;
    }
  }

  /**
   * Send all the queue up text to a token.
   */
  private void flushTokens(ByteBuffer outgoing)
  {
    if (outgoing.position() == 0)
      return;
    processToken(last, new String(outgoing.array(), 0, outgoing.position()));
    outgoing.rewind();
  }

  /**
   * Process a token.
   *
   * This will add it to the stack and then match the stack with the nearest
   * rule.
   *
   * @param token
   * @param value
   */
  protected void processToken(Token token, String value)
  {
    if (token == null)
      return;
    Entity entity = add(token, value);

    // Take the next lookahead
    Rule rule1 = this.lookahead;
    this.lookahead = null;
    if (rule1 != null)
    {
//      System.out.println("      FORWARD " + rule1);
      if (rule1.apply(this, entity))
      {
        return;
      }
    }

    // If not handled then proceed to rules.
    boolean done = false;
    while (!done && !stack.isEmpty())
    {
      done = true;
      for (Rule rule : state.getRules())
      {
//        System.out.println("      RULE " + rule);
        if (rule.apply(this, stack.getLast()))
        {
          done = false;
          break;
        }
      }
    }
  }

  /**
   * Add a token to the token stack
   *
   * @param token
   * @param object
   * @return
   */
  public Entity add(Token token, String object)
  {
    Entity entity = new Entity(token, object);
    this.stack.add(entity);
    return entity;
  }

  public void error(String bad_token)
  {
    throw new RuntimeException("bad_token");
  }

  public interface State
  {

    Token[] getTokens();

    Rule[] getRules();
  }

  public interface Token
  {

    int ordinal();

    public boolean matches(byte b);

    public boolean runs();
  }

  public interface Rule
  {

    boolean apply(Parser<?> parser, Entity entity);
  }

  public interface Grammar
  {

    /**
     * Should set the initial state.
     *
     * @param p
     */
    public void start(Parser p);

    /**
     * Should check the state of the stack, fail if bad, or return the final
     * object if good.
     *
     * @param p
     * @return
     */
    public Object end(Parser p);
  }

  /**
   * Token or text.
   */
  public static class Entity
  {

    public Token token;
    public Object value;

    private Entity(Token token)
    {
      this.token = token;
    }

    private Entity(Token token, String value)
    {
      this.token = token;
      this.value = value;
    }

    @Override
    public String toString()
    {
      if (value == null)
        return token.toString();
      return value.toString();
    }
  }

  //<editor-fold desc="common">
  /**
   * Generic matcher for multiple tokens on the stack.
   */
  abstract static class MatchRule implements Rule
  {

    Token[] pattern;

    MatchRule(Token... tokens)
    {
      this.pattern = tokens;
    }

    @Override
    public boolean apply(Parser parser, Entity entity)
    {
      LinkedList<Entity> stack = parser.stack;
      int n = stack.size();
      if (n < pattern.length)
        return false;
      ListIterator<Entity> iter = stack.listIterator(stack.size());
      for (int i = 0; i < pattern.length; ++i)
      {
        if (!iter.hasPrevious())
          return false;
        Entity next = iter.previous();
        if (next.token != pattern[pattern.length - i - 1])
        {
          return false;
        }
      }
      execute(parser);
      return true;
    }

    abstract public void execute(Parser parser);
  }

//</editor-fold>
}
