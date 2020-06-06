package org.jpype.javadoc;

import org.w3c.dom.Node;

public class JavadocException extends RuntimeException
{

  public Node node;

  public JavadocException(Node node, Throwable th)
  {
    super(th);
    this.node = node;
  }
}
