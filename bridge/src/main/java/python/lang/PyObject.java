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
package python.lang;

/**
 * PyObject is a representation of a generic object in Python.
 *
 * This object will have very limited behaviors. To accept a specific behavior
 * of a Python object use one of the "as" functions.
 *
 * @author nelson85
 */
public interface PyObject
{
    /** Get the type of this object.
     * 
     * Equivalent of type(obj).
     * 
     * @return the object type.
     */
    PyType getType();

    boolean isInstance(PyObject cls);

    /**
     * Get the attributes of the object.
     *
     * @return an attribute interface for this object.
     */
    PyAttributes asAttributes();

    /**
     * Treat this object as a function.
     *
     * FIXME if the object isn't callable what should we do?
     *
     * @return a function interface.
     */
    PyCallable asFunc();

    /**
     * Treat the object as a sequence.
     *
     * @return
     */
    PySequence asSequence();

    // conversions
    int toInt();

    double toFloat();

    boolean toBool();

//  // attributes
//  boolean hasAttr(String s);
//
//  PyObject getAttr(String s);
//
//  void setAttr(String s, Object obj);
//
//  void delAttr(String s);
    // Executable
    //PyObject call(PyTuple args, PyDict kwargs);
    // dict like
//  PyObject setItem(PyObject key, Object value);
//
//  PyObject getItem(PyObject key);
//
//  void delItem(PyObject obj);
//
//  int len();
    PyObject dir();

    // logial  
    boolean not();

    boolean isTrue();

    int hash();

    PyObject str();

    PyObject repr();

    PyObject bytes();

}
