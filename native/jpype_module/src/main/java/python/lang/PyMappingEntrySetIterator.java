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

import org.jpype.internal.Utility;
import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.function.BiFunction;
import org.jpype.bridge.Interpreter;
import python.lang.PyBuiltIn;
import python.lang.PyObject;
import python.lang.PyTuple;

/**
 * Iterator implementation for iterating over the entries of a Python
 * mapping.The {@code PyMappingEntrySetIterator} class provides an iterator for
 * Python mappings, allowing Java code to traverse the key-value pairs of a
 * Python map.
 *
 * This iterator is designed to work seamlessly with Python's iteration
 * protocols and integrates with the JPype library's Python-to-Java bridge.
 *
 * <p>
 * Key features:
 * <ul>
 * <li>Supports iteration over Python mapping entries as {@link Map.Entry}
 * objects</li>
 * <li>Provides a mechanism for updating mapping entries via a custom
 * setter</li>
 * <li>Handles Python iteration semantics, including stop iteration</li>
 * </ul>
 *
 * <p>
 * Usage Example:
 * <pre>
 * PyMapping map = BuiltIn.dict();  // Python mapping object
 * Iterator&lt;Object, PyObject&gt; iterator = map.iterator();
 * while (iterator.hasNext()) {
 *     Map.Entry&lt&Object, PyObject&gt; entry = iterator.next();
 *     System.out.println("Key: " + entry.getKey() + ", Value: " + entry.getValue());
 * }
 * </pre>
 *
 * @param <K> The getType of keys in the mapping.
 * @param <V> The getType of values in the mapping.
 */
class PyMappingEntrySetIterator<K extends PyObject, V extends PyObject> implements Iterator<Map.Entry<K, V>>
{

  /**
   * The Python mapping object being iterated over.
   */
  private final PyMapping<K, V> map;

  /**
   * The Python iterator for the mapping entries.
   */
  private final PyIter<K> iter;

  /**
   * The current value yielded by the Python iterator.
   */
  private PyObject yield;

  /**
   * Indicates whether the iteration is complete.
   */
  private boolean done = false;

  /**
   * Tracks whether the next element has been checked.
   */
  private boolean check = false;

  /**
   * A custom setter function used to update mapping entries.
   */
  private final BiFunction<K, V, V> setter;

  /**
   * Constructs a new {@code PyMappingEntrySetIterator}.
   *
   * @param map The Python mapping object to iterate over.
   * @param iter The Python iterator for the mapping entries.
   */
  public PyMappingEntrySetIterator(PyMapping<K, V> map, PyIter<K> iter)
  {
    this.map = map;
    this.iter = iter;
    this.setter = this::set;
  }

  /**
   * Updates the value of a mapping entry and returns the previous value.
   *
   * This method is used as the setter function for
   * {@link Utility.MapEntryWithSet}.
   *
   * @param key The key of the mapping entry to update.
   * @param value The new value to associate with the key.
   * @return The previous value associated with the key.
   */
  private V set(K key, V value)
  {
    @SuppressWarnings("element-type-mismatch")
    var out = map.get(key);
    map.putAny(key, value);
    return out;
  }

  /**
   * Checks whether there are more elements to iterate over.
   *
   * This method determines if the Python iterator has more entries. It handles
   * Python's stop iteration semantics and ensures proper integration with
   * Java's {@link Iterator} interface.
   *
   * @return {@code true} if there are more elements to iterate over,
   * {@code false} otherwise.
   */
  @Override
  public boolean hasNext()
  {
    if (done)
      return false;
    if (check)
      return !done;
    check = true;
    if (yield == null)
      yield = PyBuiltIn.next(iter, Interpreter.stop);
    done = (yield == Interpreter.stop);
    return !done;
  }

  /**
   * Returns the next mapping entry in the iteration.
   *
   * This method retrieves the next key-value pair from the Python iterator and
   * wraps it in a {@link Map.Entry} object. If the iteration is complete, it
   * throws a {@link NoSuchElementException}.
   *
   * @return The next mapping entry as a {@link Map.Entry} object.
   * @throws NoSuchElementException If there are no more elements to iterate
   * over.
   */
  @Override
  @SuppressWarnings("unchecked")
  public Map.Entry<K, V> next() throws NoSuchElementException
  {
    if (!check)
      hasNext();
    if (done)
      throw new NoSuchElementException();
    check = false;
    // The yielded object has two members: key and value
    PyTuple tuple = (PyTuple) yield;
    PyObject key = tuple.get(0);
    PyObject value = tuple.get(1);
    return new Utility.MapEntryWithSet(key, value, setter);
  }
}
