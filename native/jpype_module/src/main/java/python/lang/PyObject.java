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
package python.lang;

import python.protocol.PyAttributes;

/**
 * PyObject is a representation of a generic object in Python.
 *
 * PyObject when created inherit from multiple Java interfaces called protocols
 * based on their duck type behavior. Use `instanceof` and casting to access the
 * available behaviors. Specific Java like behaviors are implemented on the
 * protocols where applicable.
 *
 */
public interface PyObject
{

  static PyType type()
  {
    return (PyType) PyBuiltIn.eval("object", null, null);
  }

  /**
   * Apply the attributes protocol to this object.
   *
   * This method never fails.
   *
   * @return an attribute protocol.
   */
  default PyAttributes getAttributes()
  {
    return new PyAttributes(this);
  }

  @Override
  int hashCode();

  @Override
  boolean equals(Object obj);

}
