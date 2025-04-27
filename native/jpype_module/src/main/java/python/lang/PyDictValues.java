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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import org.jpype.bridge.Backend;
import org.jpype.bridge.Interpreter;

/**
 * Represents a view of the values in a Python dictionary ({@code PyDict}) as a
 * Java {@code Collection}.
 *
 * <p>
 * This class provides a bridge between Python's {@code dict.values()} and
 * Java's {@code Collection<PyObject>}. It allows querying and manipulation of
 * Python dictionary values using Java's collection interface.</p>
 *
 * <p>
 * <b>Note:</b></p>
 * <ul>
 * <li>This class is backed by a Python dictionary ({@code PyDict}) and
 * interacts with the Python interpreter's backend.</li>
 * <li>Some operations, such as {@code add}, {@code remove}, {@code removeAll},
 * and {@code retainAll}, are unsupported because Python's {@code dict.values()}
 * view does not allow direct modification of values.</li>
 * </ul>
 *
 * <p>
 * Supported operations include:</p>
 * <ul>
 * <li>Checking if a value exists ({@code contains})</li>
 * <li>Iterating over values ({@code iterator})</li>
 * <li>Clearing all keys and values in the dictionary ({@code clear})</li>
 * <li>Querying the size of the values collection ({@code size})</li>
 * <li>Converting the values to an array ({@code toArray})</li>
 * </ul>
 *
 */
public class PyDictValues implements Collection<PyObject>
{

  /**
   * Backend interface for interacting with the Python interpreter.
   */
  private final Backend backend;

  /**
   * The Python dictionary ({@code PyDict}) whose values are represented by this
   * class.
   */
  private final PyDict dict;

  /**
   * A Python object representing the {@code dict.values()} view.
   */
  private final PyObject values;

  /**
   * Constructs a new {@code PyDictValues} instance for the given Python
   * dictionary.
   *
   * @param dict The Python dictionary ({@code PyDict}) whose values are to be
   * represented.
   */
  public PyDictValues(PyDict dict)
  {
    this.dict = dict;
    this.backend = Interpreter.getBackend();
    this.values = backend.values(dict);
  }

  /**
   * Unsupported operation. Python's {@code dict.values()} does not support
   * adding new values directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean add(PyObject e)
  {
    throw new UnsupportedOperationException("Values does not support item assignment");
  }

  /**
   * Unsupported operation. Python's {@code dict.values()} does not support
   * adding new values directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean addAll(Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException("Values does not support item assignment");
  }

  /**
   * Clears all keys and values from the Python dictionary.
   */
  @Override
  public void clear()
  {
    dict.clear();
  }

  /**
   * Checks whether the specified value exists in the dictionary.
   *
   * @param o The value to check.
   * @return {@code true} if the value exists, {@code false} otherwise.
   */
  @Override
  public boolean contains(Object o)
  {
    return backend.contains(this, o);
  }

  /**
   * Checks whether all values in the given collection exist in the dictionary.
   *
   * @param collection The collection of values to check.
   * @return {@code true} if all values exist, {@code false} otherwise.
   */
  @Override
  public boolean containsAll(Collection<?> collection)
  {
    // For large collections we should create a set first and test for subsets.
    // Slow iterative method.
    for (Iterator<?> it = collection.iterator(); it.hasNext();)
    {
      Object o = it.next();
      if (!this.contains(o))
      {
        return false;
      }
    }
    return true;
  }

  /**
   * Checks whether the dictionary has no values.
   *
   * @return {@code true} if the dictionary is empty, {@code false} otherwise.
   */
  @Override
  public boolean isEmpty()
  {
    return backend.len(this) == 0;
  }

  /**
   * Returns an iterator over the dictionary's values.
   *
   * @return An iterator over the values in the dictionary.
   */
  @Override
  public Iterator<PyObject> iterator()
  {
    return (Iterator<PyObject>) PyBuiltIn.iter(values).iterator();
  }

  /**
   * Unsupported operation. Python's {@code dict.values()} does not support
   * removing values directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean remove(Object o)
  {
    throw new UnsupportedOperationException("PyDict values does not support item removal");
  }

  /**
   * Unsupported operation. Python's {@code dict.values()} does not support
   * removing values directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean removeAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("PyDict values does not support item removal");
  }

  /**
   * Unsupported operation. Python's {@code dict.values()} does not support
   * modifying values directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("PyDict values does not support item removal");
  }

  /**
   * Returns the number of values in the dictionary.
   *
   * @return The size of the values collection.
   */
  @Override
  public int size()
  {
    return backend.len(values);
  }

  /**
   * Converts the dictionary's values to an array.
   *
   * @return An array containing the dictionary's values.
   */
  @Override
  public Object[] toArray()
  {
    return new ArrayList(PyBuiltIn.list(values)).toArray();
  }

  /**
   * Converts the dictionary's values to an array of the specified type.
   *
   * @param a The array into which the values are to be stored.
   * @return An array containing the dictionary's values.
   */
  @Override
  public <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList(PyBuiltIn.list(values)).toArray(a);
  }
}
