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
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;
import python.protocol.PyIter;

/**
 * Conversion of a Python iterator to Java.
 *
 * This is a private class used under the hood.
 *
 * Python and Java iterators don't share similar design philosophies, so we will
 * need to keep some state on the Java side to manage the conversion.
 */
public class PyIterator implements Iterator<PyObject>
{

  private final PyIter iter;
  private PyObject yield;
  private boolean done = false;
  private boolean check = false;

  public PyIterator(PyIter iter)
  {
    this.iter = iter;
  }

  @Override
  public boolean hasNext()
  {
    if (done)
      return false;
    if (check)
      return !done;
    check = true;
    if (yield == null)
      yield = BuiltIn.next(iter, Interpreter.stop);
    done = (yield == Interpreter.stop);
    return !done;
  }

  @Override
  public PyObject next() throws NoSuchElementException
  {
    if (!check)
      hasNext();
    if (done)
      throw new NoSuchElementException();
    check = false;
    return yield;
  }

}
