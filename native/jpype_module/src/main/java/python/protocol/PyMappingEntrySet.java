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
import java.util.Map;
import java.util.Set;
import python.lang.PyBuiltIn;
import python.lang.PyObject;

/**
 * A representation of the entry set of a Python mapping, implementing the
 * {@link Set} interface.
 *
 * <p>
 * This class provides a view of the key-value pairs in a Python mapping
 * (`PyMapping`) as a Java {@link Set} of {@link Map.Entry}. It allows
 * operations such as adding entries, clearing the mapping, and iterating over
 * entries.
 *
 * <p>
 * Note: This class is a wrapper around a {@link PyMapping} instance and
 * delegates most operations to the underlying mapping. It is designed to
 * integrate Python-style mappings into Java collections seamlessly.
 *
 * <h2>Unsupported Operations</h2>
 * Some operations, such as {@code retainAll}, are not supported and will throw
 * {@link UnsupportedOperationException}. Additionally, methods such as
 * {@code contains} and {@code remove} are currently not implemented and always
 * return {@code false}.
 *
 * <h2>Thread Safety</h2>
 * This class does not guarantee thread safety. If the underlying
 * {@link PyMapping} is accessed concurrently, external synchronization is
 * required.
 *
 * <h2>Usage Example</h2>
 * <pre>
 * PyMapping pyMapping = ...; // Initialize a Python mapping
 * PyObject items = BuiltIn.items(pyMapping); // Get the items view
 * PyMappingEntrySet entrySet = new PyMappingEntrySet(pyMapping, items);
 *
 * // Add a new entry
 * entrySet.add(new AbstractMap.SimpleEntry<>("key", new PyObject("value")));
 *
 * // Iterate over entries
 * for (Map.Entry<Object, PyObject> entry : entrySet) {
 *     System.out.println(entry.getKey() + " -> " + entry.getValue());
 * }
 *
 * // Clear all entries
 * entrySet.clear();
 * </pre>
 *
 * @see PyMapping
 * @see Map.Entry
 * @see Set
 */
public class PyMappingEntrySet<K extends PyObject, V extends PyObject> implements Set<Map.Entry<K, V>>
{

  /**
   * The Python mapping whose entries are represented by this set.
   */
  private final PyMapping<K,V> map;

  /**
   * The Python object representing the items view of the mapping.
   */
  private final PyObject items;

  /**
   * Constructs a {@code PyMappingEntrySet} for the given Python mapping and its
   * items view.
   *
   * @param map the Python mapping whose entries will be represented as a set
   * @param items the Python object representing the items view of the mapping
   */
  PyMappingEntrySet(PyMapping<K,V> map, PyObject items)
  {
    this.map = map;
    this.items = items;
  }

  /**
   * Adds the specified entry to the mapping.
   *
   * @param e the entry to add
   * @return {@code true} if the entry was added successfully, meaning the key
   * did not previously exist in the mapping; {@code false} if the key already
   * existed and its value was updated.
   */
  @Override
  public boolean add(Map.Entry<K, V> e)
  {
    boolean previous = this.map.containsKey(e.getKey());
    this.map.putAny(e.getKey(), e.getValue());
    return !previous;
  }

  /**
   * Adds all entries in the specified collection to the mapping.
   *
   * @param c the collection of entries to add
   * @return {@code true} if all entries were added successfully
   */
  @Override
  public boolean addAll(Collection<? extends Map.Entry<K, V>> c)
  {
    for (var v : c)
    {
      this.add(v);
    }
    return true;
  }

  /**
   * Clears all entries from the mapping.
   */
  @Override
  public void clear()
  {
    this.map.clear();
  }

  /**
   * Checks if the specified entry exists in the mapping.
   *
   * <p>
   * Note: This method is not implemented and always returns {@code false}.
   *
   * @param o the entry to check
   * @return {@code false} always
   */
  @Override
  public boolean contains(Object o)
  {
    return false;
  }

  /**
   * Checks if the mapping contains all entries in the specified collection.
   *
   * <p>
   * Note: This method is not implemented and always returns {@code false}.
   *
   * @param c the collection of entries to check
   * @return {@code false} always
   */
  @Override
  public boolean containsAll(Collection<?> c)
  {
    return false;
  }

  /**
   * Checks if the mapping contains no entries.
   *
   * @return {@code true} if the mapping is empty, otherwise {@code false}
   */
  @Override
  public boolean isEmpty()
  {
    return map.isEmpty();
  }

  /**
   * Returns an iterator over the entries in the mapping.
   *
   * @return an iterator for the entry set
   */
  @Override
  public Iterator<Map.Entry<K, V>> iterator()
  {
    return new PyMappingEntrySetIterator<>(map, PyBuiltIn.iter(this.items));
  }

  /**
   * Removes the specified entry from the mapping.
   *
   * <p>
   * Note: This method is not implemented and always returns {@code false}.
   *
   * @param o the entry to remove
   * @return {@code false} always
   */
  @Override
  public boolean remove(Object o)
  {
    return false;
  }

  /**
   * Removes all entries in the specified collection from the mapping.
   *
   * <p>
   * Note: This method is not implemented and always returns {@code false}.
   *
   * @param c the collection of entries to remove
   * @return {@code false} always
   */
  @Override
  public boolean removeAll(Collection<?> c)
  {
    return false;
  }

  /**
   * Retains only the entries in the mapping that are contained in the specified
   * collection.
   *
   * <p>
   * This operation is unsupported and will throw
   * {@link UnsupportedOperationException}.
   *
   * @param c the collection of entries to retain
   * @throws UnsupportedOperationException always thrown
   */
  @Override
  public boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Returns the number of entries in the mapping.
   *
   * @return the size of the entry set
   */
  @Override
  public int size()
  {
    return this.map.size();
  }

  /**
   * Returns an array containing all entries in the mapping.
   *
   * @return an array of entries
   */
  @Override
  public Object[] toArray()
  {
    return new ArrayList<>(this).toArray();
  }

  /**
   * Returns an array containing all entries in the mapping, using the provided
   * array type.
   *
   * @param a the array into which the entries will be stored, if it is large
   * enough; otherwise, a new array of the same type will be allocated
   * @return an array containing the entries
   * @throws ArrayStoreException if the runtime type of the specified array is
   * not a supertype of the runtime type of every entry
   */
  @Override
  public <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList<>(this).toArray(a);
  }
}
