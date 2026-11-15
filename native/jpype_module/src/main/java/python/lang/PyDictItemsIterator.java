// --- file: python/lang/PyDictItemsIterator.java ---
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

import java.util.Iterator;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.function.BiFunction;
import org.jpype.MainInterpreter;
import org.jpype.internal.FunctionalAdapters;

/**
 * Iterates over the (key, value) pairs of a Python {@code dict}, backing
 * {@link PyDictItems}'s {@code iterator()}.
 *
 * @author nelson85
 */
public class PyDictItemsIterator<K, V> implements Iterator<Map.Entry<K, V>>
{

  private final PyBuiltIn builtin;
  private boolean check = false;
  private boolean done = false;
  private final PyIter<?> iter;
  private final BiFunction<K, V, V> setter;
  private PyObject yield;

  public PyDictItemsIterator(PyIter<?> iter, BiFunction<K, V, V> setter)
  {
    this.builtin = iter.builtin();
    this.iter = iter;
    this.setter = setter;
  }

  @Override
  @SuppressWarnings("unchecked")
  public boolean hasNext()
  {
    if (done)
      return false;
    if (check)
      return !done;
    check = true;
    if (yield == null)
      yield = builtin.backend.next(iter, MainInterpreter.stop);
    done = yield.equals(MainInterpreter.stop);
    return !done;
  }

  @SuppressWarnings("unchecked")
  @Override
  public Map.Entry<K, V> next() throws NoSuchElementException
  {
    if (!check)
      hasNext();
    if (done)
      throw new NoSuchElementException();
    check = false;

    K key = (K) builtin.backend.getitemSequence(yield, 0);
    V value = (V) builtin.backend.getitemSequence(yield, 1);
    yield = null;
    return new FunctionalAdapters.MapEntryWithSet<>(key, value, setter);
  }

}
