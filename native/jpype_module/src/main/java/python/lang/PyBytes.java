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

import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;

/**
 * Java front end for concrete Python bytes.
 */
public interface PyBytes extends PyObject
{
  /** 
   * Create a new ByteArray with a fixed length.
   * 
   * @param length
   * @return 
   */
  static PyBytes of(int length)
  {
     return Interpreter.getBackend().newBytes(length);
  }
  
  static PyByteArray of(Iterable<PyObject> iter )
  {
     return Interpreter.getBackend().newBytes(iter);
  }

  static PyByteArray of(PyByteArray bytes)
  {
     return Interpreter.getBackend().newBytes(bytes);
  }
  
  static PyType type()
  {
    return (PyType) BuiltIn.eval("bytes", null, null);
  }

  static PyBytes fromHex(CharSequence str)
  {
    return Interpreter.getBackend().bytesFromHex(str);
  }

  PyObject decode(PyObject encoding, PyObject delete);

  PyObject translate(PyObject table);
}
