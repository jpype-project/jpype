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

import java.util.Objects;
import python.protocol.PyAttributes;

/**
 * Java front end for a Python wrapped Java object.
 */
public class PyJavaObject implements PyObject
{

  Object obj_;

  public PyJavaObject(Object obj)
  {
    this.obj_ = obj;
  }

  @Override
  public PyAttributes getAttributes()
  {
    // Java objects don't support Python attributes directly.
    throw new UnsupportedOperationException();
  }

  public Object get()
  {
    return obj_;
  }

  @Override
  public int hashCode()
  {
    return obj_.hashCode();
  }

  @Override
  public boolean equals(Object obj)
  {
    if (this == obj)
      return true;
    if (obj == null)
      return false;
    if (getClass() != obj.getClass())
      return false;
    final PyJavaObject other = (PyJavaObject) obj;
    return Objects.equals(this.obj_, other.obj_);
  }

}
