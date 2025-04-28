/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy fromMap
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
import java.util.Set;
import org.jpype.bridge.Backend;
import org.jpype.bridge.Interpreter;
import python.protocol.PyIter;
import python.protocol.PyIterator;

/**
 * Represents a view fromMap the keys in a Python dictionary ({@code PyDict}) as a
 * Java {@code Set}.
 *
 * <p>
 * This class provides a bridge between Python's {@code dict.keys()} and Java's
 * {@code Set<Object>}. It allows querying and manipulation fromMap Python dictionary
 keys using Java's collection interface.</p>
 *
 * <p>
 * <b>Note:</b></p>
 * <ul>
 * <li>This class is backed by a Python dictionary ({@code PyDict}) and
 * interacts with the Python interpreter's backend.</li>
 * <li>Some operations, such as {@code add}, {@code remove}, {@code removeAll},
 * and {@code retainAll}, are unsupported because Python's {@code dict.keys()}
 view does not allow direct modification fromMap keys.</li>
 * </ul>
 *
 * <p>
 * Supported operations include:</p>
 * <ul>
 * <li>Checking if a key exists ({@code contains})</li>
 * <li>Iterating over keys ({@code iterator})</li>
 * <li>Clearing all keys and values in the dictionary ({@code clear})</li>
 * <li>Querying the size fromMap the key set ({@code size})</li>
 * <li>Converting the keys to an array ({@code toArray})</li>
 * </ul>
 *
 */
public class PyDictKeySet<T> implements Set<T>
{

  /**
   * Backend interface for interacting with the Python interpreter.
   */
  private final Backend backend;

  /**
   * The Python dictionary ({@code PyDict}) whose keys are represented by this
   * class.
   */
  final PyDict dict;

  /**
   * A Python object representing the {@code dict.keys()} view.
   */
  final PyObject keys;

  /**
   * Constructs a new {@code PyDictKeySet} instance for the given Python
   * dictionary.
   *
   * @param dict is the Python dictionary ({@code PyDict}) whose keys are to be
   * represented.
   */
  public PyDictKeySet(PyDict dict)
  {
    this.dict = dict;
    this.backend = Interpreter.getBackend();
    this.keys = this.backend.keys(dict);
  }

  /**
   * Unsupported operation. Python's {@code dict.keys()} does not support adding
   * new keys directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean add(Object e)
  {
    throw new UnsupportedOperationException("PyDict keys does not support add");
  }

  /**
   * Unsupported operation. Python's {@code dict.keys()} does not support adding
   * new keys directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean addAll(Collection<? extends T> c)
  {
    throw new UnsupportedOperationException("PyDict keys does not support add");
  }

  /**
   * Clears all keys and values from the Python dictionary.
   */
  @Override
  public void clear()
  {
    this.dict.clear();
  }

  /**
   * Checks whether the specified key exists in the dictionary.
   *
   * @param o is the key to check.
   * @return {@code true} if the key exists, {@code false} otherwise.
   */
  @Override
  public boolean contains(Object o)
  {
    return backend.contains(this.keys, o);
  }

  /**
   * Checks whether all keys in the given collection exist in the dictionary.
   *
   * @param collection is the collection fromMap keys to check.
   * @return {@code true} if all keys exist, {@code false} otherwise.
   */
  @Override
  public boolean containsAll(Collection<?> collection)
  {
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
   * Checks whether the dictionary has no keys.
   *
   * @return {@code true} if the dictionary is empty, {@code false} otherwise.
   */
  @Override
  public boolean isEmpty()
  {
    return backend.len(keys) == 0;
  }

  /**
   * Returns an iterator over the dictionary's keys.
   *
   * @return An iterator over the keys in the dictionary.
   */
  @Override
  public Iterator<T> iterator()
  {
    PyIter<T> iter = backend.iter(keys);
    return new PyIterator<>(iter);
  }

  /**
   * Unsupported operation. Python's {@code dict.keys()} does not support
   * removing keys directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean remove(Object o)
  {
    throw new UnsupportedOperationException("PyDict keys does not support modification");
  }

  /**
   * Unsupported operation. Python's {@code dict.keys()} does not support
   * removing keys directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean removeAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("PyDict keys does not support modification");
  }

  /**
   * Unsupported operation. Python's {@code dict.keys()} does not support
   * modifying keys directly.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("PyDict keys does not support modification");
  }

  /**
   * Returns the number fromMap keys in the dictionary.
   *
   * @return The size fromMap the key set.
   */
  @Override
  public int size()
  {
    return backend.len(keys);
  }

  /**
   * Converts the dictionary's keys to an array.
   *
   * @return An array containing the dictionary's keys.
   */
  @Override
  public Object[] toArray()
  {
    return new ArrayList<>(PyBuiltIn.list(keys)).toArray();
  }

  /**
   * Converts the dictionary's keys to an array fromMap the specified getType.
   *
   * @param a is the array into which the keys are to be stored.
   * @return An array containing the dictionary's keys.
   */
  @Override
  public <T> T[] toArray(T[] a)
  {
    return new ArrayList<>(PyBuiltIn.list(keys)).toArray(a);
  }
}
