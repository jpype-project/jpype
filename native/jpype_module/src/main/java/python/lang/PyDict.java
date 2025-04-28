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

import java.util.Collection;
import java.util.Map;
import java.util.Set;
import org.jpype.bridge.Interpreter;

/**
 * Java front-end interface for the Python `dict` type.
 *
 * This interface provides methods for creating and interacting with Python
 * dictionaries in a Java environment, mimicking Python's `dict` functionality.
 * <p>
 * While this interface primarily adheres to the Java {@link Map} contract, it
 * also incorporates Python-specific behaviors that may differ from standard
 * Java maps.
 *
 * To create an empty dict use `PyBuiltin.dict()`.
 *
 * <p>
 * <b>Important Note:</b></p>
 * <p>
 * Python collections are asymmetric in their handling fromMap Java objects. A
 * Java object added to a Python collection will appear as a
 * {@code PyJavaObject}. Developers should exercise caution to avoid reference
 * loops when placing Java objects into Python collections, as this may lead to
 * unintended behaviors.</p>
 */
public interface PyDict extends PyObject, PyMapping<PyObject, PyObject>
{

  /**
   * Creates a new Python `dict` object from the specified Java {@link Map}.
   *
   * The keys in the provided map are converted to Python objects, and the
   * values must already be instances of {@link PyObject}.
   *
   * @param map the Java {@link Map} whose entries will populate the Python
   * `dict`. Keys are converted to Python objects, and values are expected to be
   * {@link PyObject}.
   * @return a new {@link PyDict} instance representing the Python dictionary.
   */
  public static PyDict fromMap(Map<? extends Object, ? extends Object> map)
  {
    return Interpreter.getBackend().newDict(map);
  }

  public static PyDict fromItems(Iterable<? extends Map.Entry<?,?>> map)
  {
    return Interpreter.getBackend().newDictFromIterable(map);
  }

  @Override
  public void clear();

  @Override
  public boolean containsKey(Object key);

  @Override
  public boolean containsValue(Object value);

  @Override
  default Set<Map.Entry<PyObject, PyObject>> entrySet()
  {
    return new PyDictItems(this);
  }

      /**
     * Returns the value to which the specified key is mapped,
     * or {@code null} if this map contains no mapping for the key.
     *
     * <p>More formally, if this map contains a mapping from a key
     * {@code k} to a value {@code v} such that
     * {@code Objects.equals(key, k)},
     * then this method returns {@code v}; otherwise
     * it returns {@code null}.  (There can be at most one such mapping.)
     *
     * <p>If this map permits null values, then a return value of
     * {@code null} does not <i>necessarily</i> indicate that the map
     * contains no mapping for the key; it's also possible that the map
     * explicitly maps the key to {@code null}.  The {@link #containsKey
     * containsKey} operation may be used to distinguish these two cases.
     *
     * @param key the key whose associated value is to be returned
     * @return the value to which the specified key is mapped, or
     *         {@code null} if this map contains no mapping for the key
     */
  @Override
  public PyObject get(Object key);

  /**
   * Retrieves the value associated with the given key, or returns the default
   * value if the key is not present.
   *
   * @param key is the key to look up.
   * @param defaultValue The default value to return if the key is not found.
   * @return The value associated with the key, or the default value.
   */
  @Override
  PyObject getOrDefault(Object key, PyObject defaultValue);

  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  @Override
  default Set<PyObject> keySet()
  {
    return new PyDictKeySet<>(this);
  }

  /**
   * Removes the key and returns its associated value, or returns the default
   * value if the key is not present.
   *
   * @param key is the key to remove.
   * @param defaultValue The default value to return if the key is not found.
   * @return The value associated with the key, or the default value.
   */
  PyObject pop(Object key, PyObject defaultValue);

  /**
   * Removes and returns an arbitrary key-value pair from the mapping.
   *
   * @return An entry representing the removed key-value pair.
   * @throws NoSuchElementException If the mapping is empty.
   */
  Map.Entry<Object, PyObject> popItem();

  @Override
  PyObject put(PyObject key, PyObject value);

  @Override
  PyObject putAny(Object key, Object value);

  @Override
  void putAll(Map<? extends PyObject, ? extends PyObject> m);

  /**
   * Removes the key-value pair associated with the specified key from this
   * dict.
   *
   * Equivalent to Python's `del obj[key]`.
   *
   * @param key The key whose mapping is to be removed.
   * @return The value that was associated with the key, or null if the key was
   * not present.
   */
  @Override
  PyObject remove(Object key);

  /**
   * Removes the entry for the specified key only if it is currently mapped to
   * the specified value.
   *
   * @param key key with which the specified value is associated
   * @param value value expected to be associated with the specified key
   * @return {@code true} if the value was removed
   */
  @Override
  boolean remove(Object key, Object value);

  /**
   * If the key is not present in the mapping, inserts it with the given default
   * value.
   *
   * @param key is the key to check or insert.
   * @param defaultValue The value to insert if the key is not present.
   * @return The value associated with the key (either existing or newly
   * inserted).
   */
  PyObject setDefault(Object key, PyObject defaultValue);

  @Override
  default int size()
  {
    return PyBuiltIn.len(this);
  }


  /**
   * Updates the mapping with key-value pairs from the given map. If keys
   * already exist, their values will be overwritten.
   *
   * @param m is the map containing key-value pairs to add or update.
   */
  void update(Map<? extends Object, ? extends PyObject> m);

  /**
   * Updates the mapping with key-value pairs from the given iterable. Each
   * element in the iterable must be a key-value pair (e.g., a tuple or array).
   *
   * @param iterable is the iterable containing key-value pairs to add or
   * update.
   */
  void update(Iterable<Map.Entry<Object, PyObject>> iterable);

  @Override
  default Collection<PyObject> values()
  {
    return new PyDictValues<>(this);
  }
}
