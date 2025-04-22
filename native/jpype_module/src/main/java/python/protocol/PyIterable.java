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
package python.protocol;

import java.util.Iterator;
import python.lang.PyIterator;
import python.lang.PyObject;

/**
 *
 * Java front end for abstract concept of Python iterable.
 *
 * FIXME reversed removed because of contract conflict with List.
 *
 * @author nelson85
 */
public interface PyIterable extends PyObject, Iterable<PyObject>
{

  boolean all();

  boolean any();

  PyIter iter();

  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator(this.iter());
  }

  PyObject map(PyCallable callable);

  PyObject max();

  PyObject min();

  PyObject sorted();

  PyObject sum();

}
