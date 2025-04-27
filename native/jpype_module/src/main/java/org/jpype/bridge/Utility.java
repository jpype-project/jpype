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
package org.jpype.bridge;

import java.util.Iterator;
import java.util.Map;
import java.util.function.BiFunction;
import java.util.function.Function;

/**
 * Utility class providing support methods and helper implementations.
 *
 * This class contains generic utility methods and nested classes that simplify
 * working with iterators and map entries with custom setter logic.
 */
public class Utility
{

  /**
   * Transforms an iterator by applying a given function to each element.
   *
   * This method creates a new iterator that maps each element of the input
   * iterator to a new value using the specified function.
   *
   * @param <T> The type of elements in the input iterator.
   * @param <R> The type of elements in the resulting iterator.
   * @param iterator The input iterator whose elements will be transformed.
   * @param function The function to apply to each element of the input
   * iterator.
   * @return A new iterator that provides transformed elements.
   */
  public static <T, R> Iterator<R> mapIterator(Iterator<T> iterator, Function<T, R> function)
  {
    return new Iterator<R>()
    {
      @Override
      public boolean hasNext()
      {
        return iterator.hasNext();
      }

      @Override
      public R next()
      {
        return function.apply(iterator.next());
      }
    };
  }

  /**
   * A custom implementation of {@link Map.Entry} that supports a custom setter
   * logic.
   *
   * This class allows the value of a map entry to be updated using a provided
   * {@link BiFunction}, which takes the key and the new value as inputs and
   * returns the updated value.
   *
   * @param <K> The type of the key.
   * @param <V> The type of the value.
   */
  public static class MapEntryWithSet<K, V> implements Map.Entry<K, V>
  {

    /**
     * The key of the map entry.
     */
    K key;

    /**
     * The custom setter logic for updating the value.
     */
    BiFunction<K, V, V> setter;

    /**
     * The current value of the map entry.
     */
    V value;

    /**
     * Constructs a new {@link MapEntryWithSet} with the specified key, value,
     * and setter logic.
     *
     * @param key The key of the map entry.
     * @param value The initial value of the map entry.
     * @param set A {@link BiFunction} that defines the custom setter logic.
     */
    public MapEntryWithSet(K key, V value, BiFunction<K, V, V> set)
    {
      this.key = key;
      this.value = value;
      this.setter = set;
    }

    /**
     * Returns the key of the map entry.
     *
     * @return The key of the map entry.
     */
    @Override
    public K getKey()
    {
      return key;
    }

    /**
     * Returns the current value of the map entry.
     *
     * @return The current value of the map entry.
     */
    @Override
    public V getValue()
    {
      return value;
    }

    /**
     * Updates the value of the map entry using the custom setter logic.
     *
     * @param newValue The new value to set.
     * @return The updated value after applying the custom setter logic.
     */
    @Override
    public V setValue(V newValue)
    {
      return setter.apply(key, newValue);
    }
  }
}
