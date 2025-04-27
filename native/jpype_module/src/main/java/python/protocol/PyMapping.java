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

import java.util.Collection;
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
 * Key Features:
 *
 * - Implements the core methods of Java's `Map` interface. - Provides default
 * implementations for common mapping operations. - Supports views for keys,
 * values, and items, similar to Python's `Mapping` API.
 *
 * This interface assumes that the underlying Python object implements the
 * `collections.abc.Mapping` protocol, which guarantees the presence of methods
 * like `keys()`, `values()`, and `items()`.
 *
 * Example Usage:
 * <pre>
 * PyMapping pyMapping = ...;  // Obtain an instance of PyMapping
 * PyObject value = pyMapping.get("key");  // Retrieve a value by key
 * Set<Object> keys = pyMapping.keySet();  // Get all keys
 * Collection<PyObject> values = pyMapping.values();  // Get all values
 * </pre>
 *
 * @see java.util.Map
 * @see collections.abc.Mapping
 */
public interface PyMapping extends PyProtocol, Map<Object, PyObject>
{

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
   * @param key The key to check for existence.
   * @return True if the key exists, false otherwise.
   */
  @Override
  boolean containsKey(Object key);

  /**
   * Checks if the specified value exists in this mapping.
   *
   * Equivalent to Python's `value in mapping.values()` syntax.
   *
   * @param value The value to check for existence.
   * @return True if the value exists, false otherwise.
   */
  @Override
  default boolean containsValue(Object value)
  {
    return Interpreter.getBackend().mappingContainsValue(this, value);
  }

  /**
   * Retrieves the value associated with the given key. Equivalent to Python's
   * `mapping[key]`.
   *
   * @param key The key to look up.
   * @return The value associated with the key, or null if the key is not
   * present.
   */
  @Override
  default PyObject get(Object key)
  {
    return Interpreter.getBackend().getitemMappingObject(this, key);
  }

  /**
   * Returns a set view of the key-value pairs contained in this mapping.
   * Equivalent to Python's `mapping.items()` method, but returns a Java `Set`.
   *
   * @return A set view of the key-value pairs.
   */
  @Override
  default Set<Entry<Object, PyObject>> entrySet()
  {

    return new PyMappingEntrySet(this, Interpreter.getBackend().items(this.asObject()));
  }

  /**
   * Checks if this mapping is empty (contains no key-value pairs). Equivalent
   * to Python's `len(mapping) == 0`.
   *
   * @return True if the mapping is empty, false otherwise.
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
   * @return A set view of the keys.
   */
  @Override
  default Set<Object> keySet()
  {
    return new PyMappingKeySet(this);
  }

  /**
   * Associates the specified value with the specified key in this mapping. If
   * the mapping previously contained a value for the key, the old value is
   * replaced. Equivalent to Python's `mapping[key] = value`.
   *
   * @param key The key with which the specified value is to be associated.
   * @param value The value to associate with the key.
   * @return The previous value associated with the key, or null if there was no
   * mapping for the key.
   */
  @Override
  default PyObject put(Object key, PyObject value)
  {
    return Interpreter.getBackend().setitemFromObject(this, key, value);
  }

  default PyObject putAny(Object key, Object value)
  {
    return Interpreter.getBackend().setitemFromObject(this, key, value);
  }

  /**
   * Copies all key-value pairs from the specified map to this mapping.
   * Equivalent to Python's `mapping.update(other_mapping)`.
   *
   * @param m The map containing key-value pairs to copy.
   */
  @Override
  void putAll(Map<? extends Object, ? extends PyObject> m);

  /**
   * Removes the key-value pair associated with the specified key from this
   * mapping. Equivalent to Python's `del mapping[key]`.
   *
   * @param key The key whose mapping is to be removed.
   * @return The value that was associated with the key, or null if the key was
   * not present.
   */
  @Override
  default PyObject remove(Object key)
  {
    return Interpreter.getBackend().delByObject(this, key);
  }

  /**
   * Returns the number of key-value pairs in this mapping. Equivalent to
   * Python's `len(mapping)`.
   *
   * @return The number of key-value pairs in the mapping.
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
   * @return A collection view of the values.
   */
  @Override
  default Collection<PyObject> values()
  {
    return new PyMappingValues(this);
  }
}
