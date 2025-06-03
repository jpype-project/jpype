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

import java.util.Iterator;
import java.util.List;
import python.lang.PyBuiltIn;
import org.jpype.bridge.Interpreter;
import static python.lang.PyBuiltIn.backend;
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
public interface PySequence<T extends PyObject> extends PyCollection<T>, List<T>
{

  @Override
  default boolean contains(Object obj)
  {
    return backend().contains(this, obj);
  }

  /**
   * Retrieves an item from the sequence by its index.
   *
   * This method is equivalent to Python's `getitem(obj, index)` operation.
   *
   * @param index the index of the item to retrieve
   * @return the item at the specified index as a {@link PyObject}
   */
  @Override
  @SuppressWarnings("unchecked")
  default T get(int index)
  {
    return (T) backend().getitemSequence(this, index);
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
    return backend().getitemMappingObject(this, index);
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
   * @throws IllegalArgumentException if the type does not support tuple assignment.
   */
  default PyObject get(PyIndex... indices)
  {
    return backend().getitemMappingObject(this, PyBuiltIn.indices(indices));
  }

  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  @Override
  default Iterator<T> iterator()
  {
    return new PyIterator<>(this.iter());
  }

  /**
   * Removes an item from the sequence at the specified index.
   *
   * This method is equivalent to Python's `delitem(obj, index)` operation.
   *
   * @param index the index of the item to remove
   * @return the removed item as a {@link PyObject}
   */
  @Override
  T remove(int index);

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
  T set(int index, T value);
  
  void setAny(Object index, Object values);
  
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
    return backend().len(this);
  }
}
