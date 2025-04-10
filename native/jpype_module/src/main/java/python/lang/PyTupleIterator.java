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

import java.util.ListIterator;
import java.util.NoSuchElementException;

/**
 *
 * @author nelson85
 */
class PyTupleIterator implements ListIterator<PyObject>
{

  PyTuple tuple;
  int index;
  boolean forward = false;
  boolean reverse = false;

  PyTupleIterator(PyTuple list, int index)
  {
    this.tuple = list;
    this.index = index;
  }

  @Override
  public void add(PyObject e)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean hasNext()
  {
    return index < tuple.size();
  }

  @Override
  public boolean hasPrevious()
  {
    return index > 0;
  }

  @Override
  public PyObject next()
  {
    if (!hasNext())
      throw new NoSuchElementException();
    PyObject out = tuple.get(index);
    index++;
    forward = true;
    reverse = false;
    return out;
  }

  @Override
  public int nextIndex()
  {
    return index;
  }

  @Override
  public PyObject previous()
  {
    if (!hasPrevious())
      throw new NoSuchElementException();
    index--;
    PyObject out = tuple.get(index);
    forward = false;
    reverse = true;
    return out;
  }

  @Override
  public int previousIndex()
  {
    return index--;
  }

  @Override
  public void remove()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public void set(PyObject e)
  {
    throw new UnsupportedOperationException();
  }

}
