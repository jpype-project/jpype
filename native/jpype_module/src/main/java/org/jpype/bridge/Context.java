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
package org.jpype.bridge;

import python.lang.PyDict;
import python.lang.PyObject;

/**
 * Represents an scope of variables in the Python interpreter.
 *
 * We can consider these to be modules houses in Java space. Each has its own
 * set of variables. Python modules are shared between all scopes as they exist
 * globally and we only have one interpreter.
 *
 * When wrapping a Python module the Java wrapper class should hold its own
 * private scope object.
 *
 */
public class Context extends BuiltIn
{

  public final PyDict globalsDict;
  public final PyDict localsDict;

  public Context()
  {
    this.globalsDict = Bridge.backend.dict();
    this.localsDict = globalsDict;
  }

  public Context(PyDict globals, PyDict locals)
  {
    this.globalsDict = globals;
    this.localsDict = locals;
  }

  public PyObject eval(String source)
  {
    return Bridge.backend.eval(source, globalsDict, localsDict);
  }

  public void exec(String source)
  {
    Bridge.backend.exec(source, globalsDict, localsDict);
  }

  public void importModule(String module)
  {
    Bridge.backend.exec(String.format("import %s", module), globalsDict, localsDict);
  }

  public void importModule(String module, String as)
  {
    Bridge.backend.exec(String.format("import %s as %s", module, as), globalsDict, localsDict);
  }

  public PyObject get(String key)
  {
    return Bridge.backend.getitem(globalsDict, key);
  }

  public void set(String key, Object value)
  {
    Bridge.backend.setitem(globalsDict, key, value);
  }

  PyDict globals()
  {
    return globalsDict;
  }

  PyDict locals()
  {
    return localsDict;
  }

}
