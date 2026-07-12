// --- file: python/lang/PyExc.java ---
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

import java.io.Serializable;
import python.exceptions.PyException;

/**
 * Native version of a Python exception.
 *
 * This will be the type that is unwrapped to in Python.
 */
public interface PyExc extends PyObject, Serializable
{

  /**
   * Used to pass an exception through the Python stack.
   *
   * @param th
   * @return
   */
  static public PyExc unwrap(Throwable th)
  {
    if (th instanceof PyException)
      return ((PyException) th).get();
    return null;
  }

  String getMessage();

}
