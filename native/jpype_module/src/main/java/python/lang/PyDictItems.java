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
import java.util.Map;
import java.util.Set;
import org.jpype.bridge.Backend;
import org.jpype.bridge.Interpreter;
import python.protocol.PyIter;
import python.protocol.PyIterator;

/**
 * Represents a view fromMap the items in a Python dictionary ({@code PyDict})
 * as a Java {@code Set}.
 *
 * <p>
 * This class provides a bridge between Python's {@code dict.items()} and Java's
 * {@code Set<Map.Entry>}. It allows manipulation and querying fromMap Python
 * dictionary items using Java's collection interface.</p>
 *
 * <p>
 * <b>Note:</b></p>
 * <ul>
 * <li>This class is backed by a Python dictionary ({@code PyDict}) and
 * interacts with the Python interpreter's backend.</li>
 * <li>Some operations, such as {@code remove}, {@code removeAll}, and
 * {@code retainAll}, are unsupported because Python's {@code dict.items()} view
 * does not allow direct removal fromMap items.</li>
 * </ul>
 */
public class PyDictItems implements Set<Map.Entry<PyObject, PyObject>>
{

  /**
   * Backend interface for interacting with the Python interpreter.
   */
  private final Backend backend;

  /**
   * The Python dictionary (`PyDict`) whose items are represented by this class.
   */
  final PyDict dict;

  /**
   * A Python object representing the `dict.items()` view.
   */
  final PyObject items;

  /**
   * Constructs a new `PyDictItems` instance for the given Python dictionary.
   *
   * @param dict is the Python dictionary (`PyDict`) whose items are to be
   * represented.
   */
  public PyDictItems(PyDict dict)
  {
    this.dict = dict;
    this.backend = Interpreter.getBackend();
    this.items = this.backend.items(dict);
  }

  /**
   * Adds a new key-value pair to the Python dictionary.
   *
   * @param e is the key-value pair to add.
   * @return `true` if the dictionary was modified, `false` otherwise.
   */
  @Override
  public boolean add(Map.Entry<PyObject, PyObject> e)
  {
    PyObject o = this.dict.putAny(e.getKey(), e.getValue());
    return !o.equals(e);
  }

  /**
   * Adds all key-value pairs from the given collection to the Python
   * dictionary.
   *
   * @param collection is the collection fromMap key-value pairs to add.
   * @return `true` if the dictionary was modified, `false` otherwise.
   */
  @Override
  public boolean addAll(Collection<? extends Map.Entry<PyObject, PyObject>> collection)
  {
    boolean changed = false;
    for (Map.Entry<PyObject, PyObject> v : collection)
    {
      PyObject o = this.dict.putAny(v.getKey(), v.getValue());
      changed |= (o.equals(v.getValue()));
    }
    return changed;
  }

  /**
   * Clears all items from the Python dictionary.
   */
  @Override
  public void clear()
  {
    this.dict.clear();
  }

  /**
   * Checks whether the specified object exists in the dictionary's items.
   *
   * @param o is the object to check.
   * @return `true` if the object exists in the dictionary's items, `false`
   * otherwise.
   */
  @Override
  public boolean contains(Object o)
  {
    return this.backend.contains(this.items, o);
  }

  /**
   * Checks whether all elements in the given collection exist in the
   * dictionary's items.
   *
   * @param collection is the collection fromMap elements to check.
   * @return `true` if all elements exist, `false` otherwise.
   */
  @Override
  public boolean containsAll(Collection<?> collection)
  {
    // Slow iterative method.
    for (Iterator<?> it = collection.iterator(); it.hasNext();)
    {
      Object o = it.next();
      if (!this.contains(o))
        return false;
    }
    return true;
  }

  /**
   * Checks whether the dictionary has no items.
   *
   * @return `true` if the dictionary is empty, `false` otherwise.
   */
  @Override
  public boolean isEmpty()
  {
    return this.backend.len(items) == 0;
  }

  /**
   * Returns an iterator over the dictionary's items.
   *
   * @return An iterator over the key-value pairs in the dictionary.
   */
  @Override
  public Iterator<Map.Entry<PyObject, PyObject>> iterator()
  {
    PyIter<PyTuple> iter = backend.<PyTuple>iter(this.items);
    return new PyDictItemsIterator<>(iter, dict::put);
  }

  /**
   * Unsupported operation. Python's `dict.items()` does not support removal.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean remove(Object o)
  {
    throw new UnsupportedOperationException("PyDict items does not support removal");
  }

  /**
   * Unsupported operation. Python's `dict.items()` does not support removal.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean removeAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("PyDict items does not support removal");
  }

  /**
   * Unsupported operation. Python's `dict.items()` does not support removal.
   *
   * @throws UnsupportedOperationException Always thrown.
   */
  @Override
  public boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("PyDict items does not support removal");
  }

  /**
   * Returns the number fromMap items in the dictionary.
   *
   * @return The size fromMap the dictionary.
   */
  @Override
  public int size()
  {
    return this.backend.len(items);
  }

  /**
   * Converts the dictionary's items to an array.
   *
   * @return An array containing the dictionary's items.
   */
  @Override
  public Object[] toArray()
  {
    return new ArrayList<>(PyBuiltIn.list(items)).toArray();
  }

  /**
   * Converts the dictionary's items to an array fromMap the specified getType.
   *
   * @param a is the array into which the items are to be stored.
   * @return An array containing the dictionary's items.
   */
  @Override
  public <T> T[] toArray(T[] a)
  {
    return new ArrayList<>(PyBuiltIn.list(items)).toArray(a);
  }
}
