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

  /**
   * Apply the attributes protocol to this object.
   * 
   * The PyAttributes provides access to getattr, setattr, and delattr.
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

    /**
   * Returns a string representation of the object.
   * <p>
   * This method generates a string representation of the object, equivalent to
   * Python's {@code str(object)} operation.
   * </p>
   *
   * @return a string representation of the slice.
   */
  @Override
  String toString();
  
}
