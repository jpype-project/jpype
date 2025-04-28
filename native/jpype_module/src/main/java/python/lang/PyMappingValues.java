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
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.function.Function;
import org.jpype.bridge.Backend;
import org.jpype.bridge.Interpreter;
import static python.lang.PyBuiltIn.backend;
import python.lang.PyObject;

/**
 * Provides a Java {@link Collection} interface for the values of a Python
 * {@link PyMapping}.
 *
 * This class is specifically designed to bridge Python's
 * {@code collections.abc.Mapping} with Java's {@link Collection} interface,
 * allowing Python mappings to expose their values in a way that is compatible
 * with Java's collection APIs.
 *
 * <p>
 * It assumes that the Python mapping implements {@code __iter__} and
 * {@code __getitem__}, which are used to iterate over keys and retrieve
 * corresponding values. Complex operations are delegated to a backend
 * interface, {@link Backend}, which handles Python-specific operations.</p>
 *
 * <p>
 * Note: This class does not support adding values directly, as Python mappings
 * require key-value pairs for insertion. Unsupported operations will throw
 * {@link UnsupportedOperationException}.</p>
 *
 * @see PyMapping
 * @see Backend
 */
class PyMappingValues<K extends PyObject,V extends PyObject> implements Collection<V>
{

  private final Backend backend;
  private final PyMapping<K, V> map;

  /**
   * Constructs a {@code PyMappingValues} instance for the given Python mapping.
   *
   * @param map the Python mapping whose values are to be exposed as a Java
   * {@link Collection}
   */
  public PyMappingValues(PyMapping<K, V> map)
  {
    this.map = map;
    this.backend = backend();
  }

  /**
   * Unsupported operation.
   *
   * <p>
   * Python mappings require key-value pairs for insertion, so adding values
   * directly is not supported.</p>
   *
   * @param e the value to add (unsupported)
   * @return never returns (throws {@link UnsupportedOperationException})
   * @throws UnsupportedOperationException always thrown
   */
  @Override
  public boolean add(V e)
  {
    throw new UnsupportedOperationException("Adding values directly is not supported.");
  }

  /**
   * Unsupported operation.
   *
   * <p>
   * Python mappings require key-value pairs for insertion, so adding
   * collections of values directly is not supported.</p>
   *
   * @param collection the collection of values to add (unsupported)
   * @return never returns (throws {@link UnsupportedOperationException})
   * @throws UnsupportedOperationException always thrown
   */
  @Override
  public boolean addAll(Collection<? extends V> collection)
  {
    throw new UnsupportedOperationException("Adding collections of values directly is not supported.");
  }

  /**
   * Removes all key-value pairs from the underlying Python mapping.
   */
  @Override
  public void clear()
  {
    this.map.clear();
  }

  /**
   * Checks if the specified value exists in the underlying Python mapping.
   *
   * @param value the value to check for
   * @return {@code true} if the value exists, {@code false} otherwise
   */
  @Override
  public boolean contains(Object value)
  {
    return backend.mappingContainsValue(map, value);
  }

  /**
   * Checks if all values in the given collection exist in the underlying Python
   * mapping.
   *
   * @param collection the collection of values to check for
   * @return {@code true} if all values exist, {@code false} otherwise
   */
  @Override
  public boolean containsAll(Collection<?> collection)
  {
    return backend.mappingContainsAllValues(map, collection);
  }

  /**
   * Checks if the underlying Python mapping is empty.
   *
   * @return {@code true} if the mapping is empty, {@code false} otherwise
   */
  @Override
  public boolean isEmpty()
  {
    return this.map.isEmpty();
  }

  /**
   * Returns an {@link Iterator} over the values of the underlying Python
   * mapping.
   *
   * <p>
   * The iterator applies a transformation function to map keys to their
   * corresponding values using the {@code __getitem__} functionality of the
   * Python mapping.</p>
   *
   * @return an iterator over the values of the mapping
   */
  @Override
  public Iterator<V> iterator()
  {
    Function<K, V> function = p -> this.map.get(p);
    Iterator<?> iter = this.map.keySet().iterator();
    return Utility.mapIterator(this.map.keySet().iterator(), function);
  }

  /**
   * Removes the specified value from the underlying Python mapping.
   *
   * @param value the value to remove
   * @return {@code true} if the value was removed, {@code false} otherwise
   */
  @Override
  public boolean remove(Object value)
  {
    return backend.mappingRemoveValue(map, value);
  }

  /**
   * Removes all values in the given collection from the underlying Python
   * mapping.
   *
   * @param collection the collection of values to remove
   * @return {@code true} if any values were removed, {@code false} otherwise
   */
  @Override
  public boolean removeAll(Collection<?> collection)
  {
    return backend.mappingRemoveAllValue(map, collection);
  }

  /**
   * Retains only the values in the given collection in the underlying Python
   * mapping.
   *
   * @param collection the collection of values to retain
   * @return {@code true} if the mapping was modified, {@code false} otherwise
   */
  @Override
  public boolean retainAll(Collection<?> collection)
  {
    return backend.mappingRetainAllValue(map, collection);
  }

  /**
   * Returns the number of key-value pairs in the underlying Python mapping.
   *
   * @return the size of the mapping
   */
  @Override
  public int size()
  {
    return this.map.size();
  }

  /**
   * Returns an array containing all the values in the mapping.
   *
   * @return an array of values
   */
  @Override
  public Object[] toArray()
  {
    return new ArrayList<>(this).toArray();
  }

  /**
   * Returns an array containing all the values in the mapping, using the
   * provided array's type.
   *
   * @param a the array into which the values will be stored
   * @param <T> the type of the array elements
   * @return an array of values
   */
  @Override
  public <T> T[] toArray(T[] a)
  {
    return new ArrayList<>(this).toArray(a);
  }
}
