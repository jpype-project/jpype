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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.Set;
import org.jpype.bridge.Backend;
import org.jpype.bridge.Interpreter;

/**
 * A representation of the key set of a Python mapping, implementing the
 * {@link Set} interface.
 *
 * <p>
 * This class provides a view of the keys in a Python mapping (`PyMapping`) as a
 * Java {@link Set}. It allows operations such as checking for key existence,
 * iterating over keys, and removing keys. However, modification operations such
 * as adding or retaining elements are unsupported, as the underlying Python
 * mapping does not support such operations.
 *
 * <p>
 * Note: This class is a wrapper around a {@link PyMapping} instance and
 * delegates most operations to the underlying mapping. It is designed to
 * integrate Python-style mappings into Java collections seamlessly.
 *
 * <h2>Unsupported Operations</h2>
 * Methods that modify the set, such as {@code add}, {@code addAll}, and
 * {@code retainAll}, will throw {@link UnsupportedOperationException}, as
 * Python mappings do not support these operations.
 *
 * <h2>Thread Safety</h2>
 * This class does not guarantee thread safety. If the underlying
 * {@link PyMapping} is accessed concurrently, external synchronization is
 * required.
 *
 * <h2>Usage Example</h2>
 * <pre>
 * PyMapping pyMapping = ...; // Initialize a Python mapping
 * PyMappingKeySet keySet = new PyMappingKeySet(pyMapping);
 *
 * // Check if a key exists
 * boolean exists = keySet.contains("someKey");
 *
 * // Iterate over keys
 * for (Object key : keySet) {
 *     System.out.println(key);
 * }
 *
 * // Remove a key
 * keySet.remove("someKey");
 * </pre>
 *
 * @version 1.0
 * @see PyMapping
 * @see Set
 */
public class PyMappingKeySet<V> implements Set<V>
{

  private final Backend backend;
  /**
   * The underlying Python mapping whose keys are represented by this set.
   */
  final PyMapping map;

  /**
   * Constructs a {@code PyMappingKeySet} for the given Python mapping.
   *
   * @param mapping the Python mapping whose keys will be represented as a set
   */
  public PyMappingKeySet(PyMapping mapping)
  {
    this.map = mapping;
    this.backend = Interpreter.getBackend();
  }

  /**
   * Unsupported operation. Adding keys is not allowed in this key set.
   *
   * @param e the element to add
   * @throws UnsupportedOperationException always thrown
   */
  @Override
  public boolean add(Object e)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Unsupported operation. Adding all keys is not allowed in this key set.
   *
   * @param c the collection of elements to add
   * @throws UnsupportedOperationException always thrown
   */
  @Override
  public boolean addAll(Collection<? extends V> c)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Clears all keys from the mapping.
   */
  @Override
  public void clear()
  {
    map.clear();
  }

  /**
   * Checks if the specified key exists in the mapping.
   *
   * @param o the key to check for existence
   * @return {@code true} if the key exists, otherwise {@code false}
   */
  @Override
  public boolean contains(Object o)
  {
    return map.containsKey(o);
  }

  /**
   * Checks if the mapping contains all keys in the specified collection.
   *
   * @param c the collection of keys to check
   * @return {@code true} if all keys exist, otherwise {@code false}
   */
  @Override
  public boolean containsAll(Collection<?> c)
  {
    for (var x : c)
    {
      if (!map.containsKey(x))
      {
        return false;
      }
    }
    return true;
  }

  /**
   * Checks if the mapping contains no keys.
   *
   * @return {@code true} if the key set is empty, otherwise {@code false}
   */
  @Override
  public boolean isEmpty()
  {
    return map.isEmpty();
  }

  /**
   * Returns an iterator over the keys in the mapping.
   *
   * @return an iterator for the key set
   */
  @Override
  public Iterator<V> iterator()
  {
    return new PyIterator(backend.iter(map));
  }

  /**
   * Removes the specified key from the mapping.
   *
   * @param o the key to remove
   * @return {@code true} if the key was removed, otherwise {@code false}
   */
  @Override
  public boolean remove(Object o)
  {
    return map.remove(o) != null;
  }

  /**
   * Removes all keys in the specified collection from the mapping.
   *
   * @param collection the collection of keys to remove
   * @return {@code true} if all keys were removed successfully, otherwise
   * {@code false}
   */
  @Override
  public boolean removeAll(Collection<?> collection)
  {
    return backend.mappingRemoveAllKeys(map, collection);
  }

  /**
   * Unsupported operation. Retaining keys is not allowed in this key set.
   *
   * @param collection the collection of elements to retain
   * @throws UnsupportedOperationException always thrown
   */
  @Override
  public boolean retainAll(Collection<?> collection)
  {
    return backend.mappingRetainAllKeys(map, collection);
  }

  /**
   * Returns the number of keys in the mapping.
   *
   * @return the size of the key set
   */
  @Override
  public int size()
  {
    return map.size();
  }

  /**
   * Returns an array containing all keys in the mapping.
   *
   * @return an array of keys
   */
  @Override
  public Object[] toArray()
  {
    return new ArrayList<>(this).toArray();
  }

  /**
   * Returns an array containing all keys in the mapping, using the provided
   * array type.
   *
   * @param a the array into which the keys will be stored, if it is large
   * enough; otherwise, a new array of the same type will be allocated
   * @return an array containing the keys
   * @throws ArrayStoreException if the runtime type of the specified array is
   * not a supertype of the runtime type of every key
   */
  @Override
  public <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList<>(this).toArray(a);
  }
}
