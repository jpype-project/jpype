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

import java.util.List;
import org.jpype.bridge.BuiltIn;
import org.jpype.bridge.Interpreter;
import python.lang.PyObject;

/**
 * Interface for objects that act as sequences.
 */
public interface PySequence extends PyProtocol, List<PyObject>
{

  /**
   * Get the item by index.
   *
   * Equivalent to getitem(obj, index).
   *
   * @param index
   * @return
   */
  @Override
  default PyObject get(int index)
  {
    return BuiltIn.sequence_getitem(index);
  }

  /**
   * Get a slice or by index.
   *
   * Equivalent to obj[slice].
   *
   * @param index
   * @return
   */
  default PyObject get(PyIndex index)
  {
    return BuiltIn.sequence_getitem(index.asObject());
  }

  /**
   * Get slices.
   *
   * Equivalent to obj[slice, slice].
   *
   *
   * @param slices
   * @return
   */
  default PyObject get(PyIndex... indices)
  {
    return Interpreter.getBackend().sequenceGetItem(BuiltIn.tuple(indices));
  }

  /**
   * Set an item by index.
   *
   * Equivalent to setitem(obj, index).
   *
   * @param index
   * @param value
   */
  @Override
  PyObject set(int index, PyObject value);

  /**
   * Delete an item by index.
   *
   * Equivalent to delitem(obj, index).
   *
   * @param index
   */
  @Override
  PyObject remove(int index);

  /**
   * Delete an item by index.
   *
   * Equivalent to len(obj).
   *
   * @return
   */
  @Override
  int size();
}
