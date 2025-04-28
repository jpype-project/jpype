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
import org.jpype.bridge.Interpreter;
import org.jpype.internal.Utility;
import python.protocol.PyIter;

/**
 *
 * @author nelson85
 */
public class PyDictItemsIterator<K,V> implements Iterator<Map.Entry<K, V>>
{
  private final PyIter<PyTuple> iter;
  private PyTuple yield;
  private boolean done = false;
  private boolean check = false;
  private final BiFunction<K, V, V> setter;
  
  public PyDictItemsIterator(PyIter<PyTuple> iter,  BiFunction<K, V, V> setter)
  {
    this.iter =iter;
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
      yield = (PyTuple) PyBuiltIn.next(iter, Interpreter.stop);
    done = (yield == Interpreter.stop);
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
    
    K key = (K) yield.get(0);
    V value = (V) yield.get(1);
    return new Utility.MapEntryWithSet<>(key, value, setter);
  }
  
}
