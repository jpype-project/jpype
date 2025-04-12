/* ****************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 * ***************************************************************************/
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
 * @author nelson85
 */
public class Scope
{

    public final Builtin builtin;
    public final PyDict globals;

    private Scope(Builtin builtin, PyDict globals)
    {
        this.builtin = builtin;
        this.globals = globals;
    }

    PyObject eval(String source)
    {
        return null;
    }

    void imports(String module)
    {

    }

    void imports(String module, String as)
    {

    }

}
