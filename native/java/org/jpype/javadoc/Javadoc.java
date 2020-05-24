package org.jpype.javadoc;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.w3c.dom.Node;

public class Javadoc
{
  public String description;
  public String ctors;
  public Map<String, String> methods = new HashMap<>();
  public Map<String, String> fields = new HashMap<>();

  // These will be removed once debugging is complete
  public Node descriptionNode;
  public List<Node> ctorsNode;
  public List<Node> methodNodes;
  public List<Node> innerNode;
  public List<Node> fieldNodes;
}
