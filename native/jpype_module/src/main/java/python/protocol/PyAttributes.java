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

import python.lang.PyDict;
import python.lang.PyList;
import python.lang.PyObject;

/**
 * FIXME try to make this more like a Java map if possible, but currently it
 * can't implement the full contract.
 *
 * @author nelson85
 */
public interface PyAttributes extends PyProtocol
{

  /**
   * Check if an attribute exists.
   *
   * Equivalent of hasattr(obj, key).
   *
   * @param key
   * @return true if the attribute exists.
   */
  boolean has(String key);

  /**
   * Get the value of an attribute.
   *
   * Equivalent of getattr(obj, key).
   *
   * @param key
   * @return
   */
  PyObject get(String key);

  /**
   * Set the value of an attribute.
   *
   * Equivalent of setattr(obj, key, value).
   *
   * @param key
   * @return
   */
  void set(String key, Object obj);

  void remove(String key);

  PyList dir();

  PyDict dict();
}
