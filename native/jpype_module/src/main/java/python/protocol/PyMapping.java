/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
package python.protocol;

import java.util.Collection;
import java.util.Map;
import java.util.Set;
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;
import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
public interface PyMapping extends PyProtocol, Map<Object, PyObject>
{

  @Override
  void clear();

  @Override
  boolean containsKey(Object key);

  @Override
  boolean containsValue(Object value);

  @Override
  default Set<Entry<Object, PyObject>> entrySet()
  {
    return new PyMappingEntrySet(this, Interpreter.getBackend().items(this.asObject()));
  }

  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  @Override
  default Set<Object> keySet()
  {
    return new PyMappingKeySet(this);
  }

  @Override
  PyObject put(Object key, PyObject value);

  @Override
  void putAll(Map<? extends Object, ? extends PyObject> m);

  @Override
  PyObject remove(Object key);

  @Override
  int size();

  @Override
  default Collection<PyObject> values()
  {
    return new PyMappingValues(this, Interpreter.getBackend().values(this.asObject()));
  }

}
