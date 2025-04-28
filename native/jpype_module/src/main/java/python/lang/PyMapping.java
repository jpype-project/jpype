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
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import python.lang.PyBuiltIn;
import org.jpype.bridge.Interpreter;
import python.lang.PyObject;

/**
 * Represents a protocol for classes registered as Python
 * `collections.abc.Mapping`.
 *
 * This interface is designed to model the behavior of Python's `Mapping`
 * abstract base class, which defines the minimal interface for mapping objects,
 * such as dictionaries, and other custom mapping types. It extends the Java
 * `Map` interface to provide additional functionality specific to Python
 * mappings.
 *
 * <p>
 * Key Features:</p>
 * <ul>
 * <li>Implements the core methods of Java's `Map` interface.</li>
 * <li>Provides default implementations for common mapping operations.</li>
 * <li>Supports views for keys, values, and items, similar to Python's `Mapping`
 * API.</li>
 * </ul>
 *
 * <p>
 * This interface assumes that the underlying Python object implements the
 * `collections.abc.Mapping` protocol, which guarantees the presence of methods
 * like `keys()`, `values()`, and `items()`.</p>
 *
 * <p>
 * Example Usage:</p>
 * <pre>
 * PyMapping pyMapping = ...;  // Obtain an instance of PyMapping
 * PyObject value = pyMapping.get("key");  // Retrieve a value by key
 * Set&lt;Object> keys = pyMapping.keySet();  // Get all keys
 * Collection&lt;PyObject> values = pyMapping.values();  // Get all values
 * </pre>
 *
 * @see java.util.Map
 * @see collections.abc.Mapping
 */
public interface PyMapping<K extends PyObject, V extends PyObject> extends PyCollection<K>, Map<K, V>
{

  @Override
  default boolean contains(Object obj)
  {
    return Interpreter.getBackend().contains(this, obj);
  }

  @Override
  default Iterator<K> iterator()
  {
    return new PyIterator<>(this.iter());
  }

  /**
   * Removes all key-value pairs from this mapping. Equivalent to Python's
   * `clear()` method.
   */
  @Override
  default void clear()
  {
    Interpreter.getBackend().mappingClear(this);
  }

  /**
   * Checks if the specified key exists in this mapping. Equivalent to Python's
   * `key in mapping` syntax.
   *
   * @param key is the key to check for existence.
   * @return true if the key exists, false otherwise.
   */
  @Override
  boolean containsKey(Object key);

  /**
   * Checks if the specified value exists in this mapping.
   *
   * Equivalent to Python's `value in mapping.values()` syntax.
   *
   * @param value is the value to check for existence.
   * @return true if the value exists, false otherwise.
   */
  @Override
  boolean containsValue(Object value);

  /**
   * Retrieves the value associated with the given key. Equivalent to Python's
   * `mapping[key]`.
   *
   * @param key is the key to look up.
   * @return the value associated with the key, or null if the key is not
   * present.
   */
  @Override
  @SuppressWarnings("unchecked")
  default V get(Object key)
  {
    return (V) Interpreter.getBackend().getitemMappingObject(this, key);
  }

  /**
   * Returns a set view of the key-value pairs contained in this mapping.
   * Equivalent to Python's `mapping.items()` method, but returns a Java `Set`.
   *
   * @return a set view of the key-value pairs.
   */
  @Override
  default Set<Entry<K, V>> entrySet()
  {
    return new PyMappingEntrySet<>(this, Interpreter.getBackend().items(this));
  }

  /**
   * Checks if this mapping is empty (contains no key-value pairs). Equivalent
   * to Python's `len(mapping) == 0`.
   *
   * @return true if the mapping is empty, false otherwise.
   */
  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Returns a set view of the keys contained in this mapping. Equivalent to
   * Python's `mapping.keys()` method, but returns a Java `Set`.
   *
   * @return a set view of the keys.
   */
  @Override
  @SuppressWarnings("unchecked")
  default Set<K> keySet()
  {
    return new PyMappingKeySet(this);
  }

  /**
   * Associates the specified value with the specified key in this mapping. If
   * the mapping previously contained a value for the key, the old value is
   * replaced. Equivalent to Python's `mapping[key] = value`.
   *
   * @param key is the key with which the specified value is to be associated.
   * @param value is the value to associate with the key.
   * @return the previous value associated with the key, or null if there was no
   * mapping for the key.
   */
  @Override
  @SuppressWarnings("unchecked")
  default V put(K key, V value)
  {
    return (V) Interpreter.getBackend().setitemFromObject(this, key, value);
  }

  default PyObject putAny(Object key, Object value)
  {
    return Interpreter.getBackend().setitemFromObject(this, key, value);
  }

  /**
   * Copies all key-value pairs from the specified map to this mapping.
   * Equivalent to Python's `mapping.update(other_mapping)`.
   *
   * @param m is the map containing key-value pairs to copy.
   */
  @Override
  void putAll(Map<? extends K, ? extends V> m);

  /**
   * Removes the key-value pair associated with the specified key from this
   * mapping. Equivalent to Python's `del mapping[key]`.
   *
   * @param key is the key whose mapping is to be removed.
   * @return the value that was associated with the key, or null if the key was
   * not present.
   */
  @Override
  V remove(Object key);

  /**
   * Returns the number of key-value pairs in this mapping. Equivalent to
   * Python's `len(mapping)`.
   *
   * @return the number of key-value pairs in the mapping.
   */
  @Override
  default int size()
  {
    return PyBuiltIn.len(this);
  }

  /**
   * Returns a collection view of the values contained in this mapping.
   * Equivalent to Python's `mapping.values()` method, but returns a Java
   * `Collection`.
   *
   * @return a collection view of the values.
   */
  @Override
  default Collection<V> values()
  {
    return new PyMappingValues<K, V>(this);
  }
}
