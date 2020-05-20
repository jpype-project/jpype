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

  Grammar grammar;
  State state = null;
  Token last = null;
  Rule lookahead = null;
  LinkedList<Entity> stack = new LinkedList<>();

  public Parser()
  {
  }

  void setGrammar(Grammar g)
  {
    this.grammar = g;
    this.state = g.initialState();
  }

  public T parse(InputStream is)
  {
    grammar.start();
    try
    {
      ByteBuffer incoming = ByteBuffer.allocate(1024);
      ByteBuffer outgoing = ByteBuffer.allocate(1024);
      ReadableByteChannel channel = Channels.newChannel(is);
      while (true)
      {
        incoming.position(0);
        int rc = channel.read(incoming);
        if (rc < 0)
          break;
        int p = incoming.position();
        incoming.rewind();
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
          if (match.runs())
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
    } catch (IOException ex)
    {
      throw new RuntimeException(ex);
    }
    return (T) grammar.end();
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

    public State initialState();

    public void start();

    public Object end();
  }

  /**
   * Token or text.
   */
  public static class Entity
  {

    public Token token;
    public Object content;

    private Entity(Token token)
    {
      this.token = token;
    }

    private Entity(Token token, String value)
    {
      this.token = token;
      this.content = value;
    }

    @Override
    public String toString()
    {
      if (content == null)
        return token.toString();
      return content.toString();
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
          return false;
      }

      execute(parser);
      return true;
    }

    abstract public void execute(Parser parser);
  }

//</editor-fold>
}
