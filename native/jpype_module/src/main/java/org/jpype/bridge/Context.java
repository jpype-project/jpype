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

import python.lang.PyBuiltIn;
import python.lang.PyDict;
import python.lang.PyObject;
import python.protocol.PyMapping;

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
public class Context extends PyBuiltIn
{

  public final PyDict globalsDict;
  public final PyObject localsDict;

  public Context()
  {
    if (!Interpreter.getInstance().isJava())
      throw new IllegalStateException("Java bridge must be active");
    this.globalsDict = Interpreter.backend.newDict();
    this.localsDict = globalsDict;
  }

  public Context(PyDict globals, PyObject locals)
  {
    if (!Interpreter.getInstance().isJava())
      throw new IllegalStateException("Java bridge must be active");
    this.globalsDict = globals;
    this.localsDict = locals;
  }

  /**
   * Evaluate a single statement in this context.
   *
   * @param source is a single Python statement.
   * @return the result of the evaluation.
   */
  public PyObject eval(String source)
  {
    return Interpreter.backend.eval(source, globalsDict, localsDict);
  }

  /**
   * Execute a block of code in this context.
   *
   * @param source
   */
  public void exec(String source)
  {
    Interpreter.backend.exec(source, globalsDict, localsDict);
  }

  public void importModule(String module)
  {
    Interpreter.backend.exec(String.format("import %s", module), globalsDict, localsDict);
  }

  public void importModule(String module, String as)
  {
    Interpreter.backend.exec(String.format("import %s as %s", module, as), globalsDict, localsDict);
  }

  /**
   * Get the globals dictionary.
   *
   * @return
   */
  PyDict globals()
  {
    return globalsDict;
  }

  /**
   * Get the locals mapping.
   *
   * @return
   */
  PyObject locals()
  {
    return localsDict;
  }

}
