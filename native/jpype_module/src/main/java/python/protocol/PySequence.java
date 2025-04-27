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
import python.lang.PyBuiltIn;
import org.jpype.bridge.Interpreter;
import python.lang.PyObject;

/**
 * Interface for Python-like objects that act as sequences.
 *
 * This interface extends {@link PyProtocol} and {@link List}, providing methods
 * for accessing, modifying, and deleting elements in a sequence. It also
 * supports Python-style slicing and indexing operations.
 *
 * Methods in this interface are designed to mimic Python's sequence protocol,
 * including operations such as `getitem`, `setitem`, and `delitem`.
 */
public interface PySequence extends PyProtocol, List<PyObject>
{

  /**
   * Retrieves an item from the sequence by its index.
   *
   * This method is equivalent to Python's `getitem(obj, index)` operation.
   *
   * @param index the index of the item to retrieve
   * @return the item at the specified index as a {@link PyObject}
   */
  @Override
  default PyObject get(int index)
  {
    return Interpreter.getBackend().getitemSequence(this, index);
  }

  /**
   * Retrieves an item or slice from the sequence using a {@link PyIndex}.
   *
   * This method is equivalent to Python's `obj[index]` operation, where the
   * index may represent a single item or a slice.
   *
   * @param index the {@link PyIndex} representing the item or slice to retrieve
   * @return the item or slice as a {@link PyObject}
   */
  default PyObject get(PyIndex index)
  {
    return Interpreter.getBackend().getitemMappingObject(this, index);
  }

  /**
   * Retrieves a slice or multiple slices from the sequence using an array of
   * {@link PyIndex}.
   *
   * This method is equivalent to Python's `obj[slice, slice]` operation,
   * supporting multidimensional slicing.
   *
   * @param indices an array of {@link PyIndex} objects representing the slices
   * to retrieve
   * @return the resulting slice(s) as a {@link PyObject}
   */
  default PyObject get(PyIndex... indices)
  {
    return Interpreter.getBackend().getitemMappingObject(this, PyBuiltIn.indices(indices));
  }

  /**
   * Sets an item in the sequence at the specified index.
   *
   * This method is equivalent to Python's `setitem(obj, index, value)`
   * operation.
   *
   * @param index the index of the item to set
   * @param value the value to assign to the specified index
   * @return the previous value at the specified index as a {@link PyObject}
   */
  @Override
  PyObject set(int index, PyObject value);

  /**
   * Removes an item from the sequence at the specified index.
   *
   * This method is equivalent to Python's `delitem(obj, index)` operation.
   *
   * @param index the index of the item to remove
   * @return the removed item as a {@link PyObject}
   */
  @Override
  PyObject remove(int index);

  /**
   * Returns the size of the sequence.
   *
   * This method is equivalent to Python's `len(obj)` operation.
   *
   * @return the number of items in the sequence
   */
  @Override
  default int size()
  {
        return Interpreter.getBackend().len(this);
  }
}
