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

import java.util.Collection;
import java.util.Map;
import java.util.Set;
import org.jpype.bridge.BuiltIn;
import org.jpype.bridge.Interpreter;
import python.protocol.PyMapping;

/**
 * Java front-end interface for the Python `dict` type. This interface provides
 * methods for creating and interacting with Python dictionaries in a Java
 * environment, mimicking Python's `dict` functionality.
 *
 * <p>
 * While this interface primarily adheres to the Java {@link Map} contract, it
 * also incorporates Python-specific behaviors that may differ from standard
 * Java maps.
 */
public interface PyDict extends PyObject, PyMapping
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
  static PyDict of(Map<Object, ? extends PyObject> map)
  {
    return Interpreter.getBackend().newDict(map);
  }
  static PyDict of(Iterable<Map.Entry<Object, ? extends PyObject>> map)
  {
    return Interpreter.getBackend().newDictFromIterable(map);
  }
  /**
   * Retrieves the Python type object for `dict`.
   *
   * This is equivalent to evaluating `type(dict)` in Python.
   *
   * @return the {@link PyType} instance representing the Python `dict` type.
   */
  static PyType type()
  {
    return (PyType) BuiltIn.eval("dict", null, null);
  }
  @Override
  public void clear();
  @Override
  public boolean containsKey(Object key);
  @Override
  public boolean containsValue(Object value);


  @Override
  public Set<Entry<Object, PyObject>> entrySet();

  @Override
  public PyObject get(Object key);
  /**
   * Retrieves the value associated with the given key, or returns the default
   * value if the key is not present.
   *
   * @param key The key to look up.
   * @param defaultValue The default value to return if the key is not found.
   * @return The value associated with the key, or the default value.
   */
  @Override
          PyObject getOrDefault(Object key, PyObject defaultValue);
          @Override
          public boolean isEmpty();
          @Override
          public Set<Object> keySet();
          /**
           * Removes the key and returns its associated value, or returns the default
           * value if the key is not present.
           *
           * @param key The key to remove.
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
          Entry<Object, PyObject> popItem();
          @Override
          public PyObject put(Object key, PyObject value);
          @Override
          public void putAll(Map<? extends Object, ? extends PyObject> m);
          @Override
          public PyObject remove(Object key);
          @Override
          public boolean remove(Object key, Object value);

  /**
   * If the key is not present in the mapping, inserts it with the given default
   * value.
   *
   * @param key The key to check or insert.
   * @param defaultValue The value to insert if the key is not present.
   * @return The value associated with the key (either existing or newly
   * inserted).
   */
  PyObject setDefault(Object key, PyObject defaultValue);
  @Override
  public int size();
  /**
   * Updates the mapping with key-value pairs from the given map. If keys
   * already exist, their values will be overwritten.
   *
   * @param m The map containing key-value pairs to add or update.
   */
  void update(Map<? extends Object, ? extends PyObject> m);
  /**
   * Updates the mapping with key-value pairs from the given iterable. Each
   * element in the iterable must be a key-value pair (e.g., a tuple or array).
   *
   * @param iterable The iterable containing key-value pairs to add or update.
   */
  void update(Iterable<Entry<Object, PyObject>> iterable);
  @Override
  public Collection<PyObject> values();
}
