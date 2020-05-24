package org.jpype.javadoc;

/**
 * The usual set of method required to work on DOM.
 *
 * DOM leaves a lot of basic stuff incomplete.
 */
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import org.w3c.dom.Node;

public class DomUtilities
{

  /**
   * Traverse all children in depth first order applying an operation.
   *
   * This is hardened against some level of DOM changes.
   *
   * @param node
   * @param operator
   * @param type
   */
  public static void traverseDFS(Node node, Consumer<Node> operator, short type)
  {
    Node child = node.getFirstChild();
    while (child != null)
    {
      // Get a referent to what we are processing next in case the tree changes.
      Node next = child.getNextSibling();

      // Apply transforms to children first
      if (child.getNodeType() == Node.ELEMENT_NODE)
        traverseDFS(child, operator, type);

      // Then process the outer element
      if (child.getNodeType() == type)
        operator.accept(child);

      // Proceed
      child = next;
    }
  }

  /**
   * Traverse the children of a node applying an operation.
   *
   * This is hardened against some level of DOM changes.
   *
   * @param node
   * @param operator
   * @param type
   */
  public static void traverseChildren(Node node, Consumer<Node> operator, short type)
  {
    Node child = node.getFirstChild();
    while (child != null)
    {
      // Get the next node to process in case this one is changed or removed.
      Node next = child.getNextSibling();
      if (child.getNodeType() == type)
        operator.accept(child);

      // Proceed.
      child = next;
    }
  }

  /**
   * Traverse all children in depth first order applying an operation.
   *
   * This is hardened against some level of DOM changes.
   *
   * @param node
   * @param operator
   * @param type
   */
  public static <T> void traverseDFS(Node node,
          BiConsumer<Node, T> operator, short type, T data)
  {
    Node child = node.getFirstChild();
    while (child != null)
    {
      // Get a referent to what we are processing next in case the tree changes.
      Node next = child.getNextSibling();

      // Apply transforms to children first
      if (child.getNodeType() == Node.ELEMENT_NODE)
        traverseDFS(child, operator, type, data);

      // Then process the outer element
      if (child.getNodeType() == type)
        operator.accept(child, data);

      // Proceed
      child = next;
    }
  }

  /**
   * Traverse the children of a node applying an operation.
   *
   * This is hardened against some level of DOM changes.
   *
   * @param node
   * @param operator
   * @param type
   */
  public static <T> void traverseChildren(Node node, BiConsumer<Node, T> operator, short type, T data)
  {
    Node child = node.getFirstChild();
    while (child != null)
    {
      // Get the next node to process in case this one is changed or removed.
      Node next = child.getNextSibling();
      if (child.getNodeType() == type)
        operator.accept(child, data);

      // Proceed.
      child = next;
    }
  }

  /**
   * Remove all attributes from a node.
   *
   * @param node
   */
  public static void clearAttributes(Node node)
  {
    while (node.getAttributes().getLength() > 0)
    {
      Node att = node.getAttributes().item(0);
      node.getAttributes().removeNamedItem(att.getNodeName());
    }
  }

  /**
   * Remove all children from a node.
   *
   * @param node
   */
  public static void clearChildren(Node node)
  {
    while (node.hasChildNodes())
      node.removeChild(node.getFirstChild());
  }

  /**
   * Determine if a block contains a new line.
   *
   * @param n
   * @return
   */
  public static boolean containsNL(Node n)
  {
    Node child = n.getFirstChild();
    while (child != null)
    {
      if (child.getNodeType() == Node.TEXT_NODE)
      {
        if (child.getNodeValue().contains("\n"))
          return true;
      }
      child = child.getNextSibling();
    }
    return false;
  }

  /**
   * Combine all text with neighbors in immediate children.
   *
   * @param node
   */
  public static void combineText(Node node)
  {
    // merge text nodes
    Node child = node.getFirstChild();
    while (child != null)
    {
      Node next = child.getNextSibling();
      if (child.getNodeType() != Node.TEXT_NODE)
      {
        child = next;
        continue;
      }
      if (next != null && next.getNodeType() == Node.TEXT_NODE)
      {
        child.setTextContent(child.getNodeValue() + next.getNodeValue());
        child.getParentNode().removeChild(next);
        continue;
      }
      child = next;
    }
  }

  /**
   * Merge the contents of a node with its parent.
   *
   * @param parent
   * @param node
   */
  public static void mergeNode(Node parent, Node node)
  {
    while (node.hasChildNodes())
    {
      parent.insertBefore(node.getFirstChild(), node);
    }
    parent.removeChild(node);
  }

  static void transferContents(Node dest, Node source)
  {
    while (source.hasChildNodes())
    {
      dest.appendChild(source.getFirstChild());
    }
  }

}
