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
