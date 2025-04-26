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

import java.util.Iterator;
import java.util.Map;
import java.util.function.BiFunction;
import java.util.function.Function;

/**
 * Support methods for the implementation.
 */
class Utility
{

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

  public static class MapEntryWithSet<K, V> implements Map.Entry<K, V>
  {

    K key;
    BiFunction<K, V, V> setter;
    V value;

    public MapEntryWithSet(K key, V value, BiFunction<K, V, V> set)
    {
      this.key = key;
      this.value = value;
      this.setter = set;
    }

    @Override
    public K getKey()
    {
      return key;
    }

    @Override
    public V getValue()
    {
      return value;
    }

    @Override
    public V setValue(V newValue)
    {
      return setter.apply(key, newValue);
    }

  }
}
