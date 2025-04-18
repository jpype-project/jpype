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

import org.jpype.bridge.BuiltIn;
import python.lang.PyObject;
import python.lang.PySlice;

/**
 * Interface for objects that act as sequences.
 */
public interface PySequence extends PyProtocol
{

  /**
   * Get the item by index.
   *
   * Equivalent to getitem(obj, index).
   *
   * @param index
   * @return
   */
  default PyObject get(int index)
  {
    return _get(index);
  }

  /**
   * Get a slice.
   *
   * Equivalent to obj[slice].
   *
   * @param slice
   * @return
   */
  default PyObject get(PySlice slice)
  {
    return _get(slice);
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
  default PyObject get(PySlice... slices)
  {
    return _get(BuiltIn.tuple(slices));
  }

  /**
   * Set an item by index.
   *
   * Equivalent to setitem(obj, index).
   *
   * @param index
   * @param value
   */
  void set(int index, Object value);

  /**
   * Delete an item by index.
   *
   * Equivalent to delitem(obj, index).
   *
   * @param index
   */
  void remove(int index);

  /**
   * Delete an item by index.
   *
   * Equivalent to len(obj).
   *
   * @return
   */
  int size();

  PyObject _get(Object object);
}
