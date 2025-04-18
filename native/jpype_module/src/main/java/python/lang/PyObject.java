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

import org.jpype.bridge.BuiltIn;
import python.protocol.PyCallable;
import python.protocol.PySequence;
import python.protocol.PyAttributes;
import python.protocol.PyMapping;
import python.protocol.PyNumber;

/**
 * PyObject is a representation of a generic object in Python.
 *
 * PyObjects are very generic and thus must be converted to a protocol using the
 * "as" methods. Specific Java like behaviors are implemented on the protocols
 * where applicable.
 *
 */
public interface PyObject
{

  static PyType type()
  {
    return (PyType) BuiltIn.eval("object", null, null);
  }

  /**
   * Get the type of this object.
   *
   * Equivalent of type(obj).
   *
   * @return the object type.
   */
  PyType getType();

  boolean isInstance(PyObject cls);

  /**
   * Apply the attributes protocol to this object.
   *
   * This method never fails.
   *
   * @return an attribute protocol.
   */
  PyAttributes asAttributes();

  /**
   * Apply the callable protocol to this object.
   *
   * The object must be callable for this to succeed.
   *
   * @return a callable protocol.
   */
  PyCallable asCallable();

  /**
   * Apply the sequence protocol to this object.
   *
   * @return a sequence protocol.
   */
  PySequence asSequence();

  /**
   * Apply the mapping protocol to this object.
   *
   * @return a mapping protocol.
   */
  PyMapping asMapping();

  /**
   * Apply the number protocol to this object.
   *
   * @return a number protocol.
   */
  PyNumber asNumber();

  PyObject bytes();

  @Override
  int hashCode();

  @Override
  boolean equals(Object obj);

}
