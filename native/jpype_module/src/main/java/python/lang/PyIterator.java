// --- file: python/lang/PyIterator.java ---
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
import java.util.NoSuchElementException;
import org.jpype.MainInterpreter;

/**
 * Adapts a Python iterator ({@link PyIter}) to Java's {@link Iterator},
 * backing the {@code iterator()} methods of the collection types in this
 * package (e.g. {@link PyAbstractSet#iterator()}). Each element is pulled
 * from Python lazily, one {@link Iterator#next()} call at a time.
 */
public class PyIterator<T> implements Iterator<T>
{

  private final PyBuiltIn builtin;
  private final PyIter<T> iter;
  private T yield;
  private boolean done = false;
  private boolean check = false;

  public PyIterator(PyIter<T> iter)
  {
    this.builtin = iter.builtin();
    this.iter = iter;
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
      yield = (T) builtin.next(iter, MainInterpreter.stop);
    // Each Python->Java round trip allocates a fresh proxy wrapper, so
    // reference identity (==) never matches even for the same underlying
    // Python object. equals() routes to Python's `==`, which for the
    // plain object() sentinel falls back to identity on the Python side.
    done = (yield != null && ((PyObject) yield).equals(MainInterpreter.stop));
    return !done;
  }

  @Override
  public T next() throws NoSuchElementException
  {
    if (!check)
      hasNext();
    if (done)
      throw new NoSuchElementException();
    check = false;
    T result = yield;
    yield = null;
    return result;
  }

}
