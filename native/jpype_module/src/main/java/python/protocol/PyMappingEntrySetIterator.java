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
import java.util.NoSuchElementException;
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;
import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
public class PyMappingEntrySetIterator implements Iterator<Map.Entry<Object, PyObject>>
{

  private final PyMapping map;
  private final PyIter iter;
  private PyObject yield;
  private boolean done = false;
  private boolean check = false;

  public PyMappingEntrySetIterator(PyMapping map, PyIter iter)
  {
    this.map = map;
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
  public Map.Entry<Object, PyObject> next() throws NoSuchElementException
  {
    if (!check)
      hasNext();
    if (done)
      throw new NoSuchElementException();
    check = false;
    //The obbject has two members
    PySequence tuple = yield.asSequence();
    PyObject key = tuple.get(0);
    PyObject value = tuple.get(1);
    return new Entry(key, value);
  }

  class Entry implements Map.Entry<Object, PyObject>
  {

    private final PyObject key;
    private PyObject value;

    private Entry(PyObject key, PyObject value)
    {
      this.key = key;
      this.value = value;
    }

    @Override
    public Object getKey()
    {
      return key;
    }

    @Override
    public PyObject getValue()
    {
      return value;
    }

    @Override
    public PyObject setValue(PyObject value)
    {
      PyObject prior = this.value;
      map.put(key, value);
      this.value = value;
      return prior;
    }

  }

}
