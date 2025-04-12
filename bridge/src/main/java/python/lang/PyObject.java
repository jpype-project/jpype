package python.lang;

/** ***************************************************************************
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
 **************************************************************************** */


/**
 *
 * @author nelson85
 */
public interface PyObject
{

  // attributes
  boolean hasAttr(String s);

  PyObject getAttr(String s);

  void setAttr(String s, Object obj);

  void delAttr(String s);

  // Executable
  PyObject call(PyTuple args, PyDict kwargs);

  // dict like
  PyObject setItem(PyObject key, Object value);

  PyObject getItem(PyObject key);

  void delItem(PyObject obj);

  int len();

  PyObject iter();

  // types
  boolean isInstance(PyObject cls);

  PyObject type();

  PyObject dir();

  // logial  
  boolean not();

  boolean isTrue();

  int hash();

  // conversions
  double asFloat();

  int asInt();

  PyObject str();

  PyObject repr();

  PyObject bytes();

}
