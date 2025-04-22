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
class PyListIterator implements ListIterator<PyObject>
{

  PyList list;
  int index;
  boolean forward = false;
  boolean reverse = false;

  PyListIterator(PyList list, int index)
  {
    this.list = list;
    this.index = index;
  }

  @Override
  public void add(PyObject e)
  {
    list.insert(index, list);
    forward = false;
    reverse = false;
  }

  @Override
  public boolean hasNext()
  {
    return index < list.size();
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
    PyObject out = list.get(index);
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
    PyObject out = list.get(index);
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
    if (!forward && !reverse)
      throw new IllegalStateException();

    if (forward)
      list.remove(index - 1);
    if (reverse)
      list.remove(index);
    forward = false;
    reverse = false;
  }

  @Override
  public void set(PyObject e)
  {
    if (!forward && !reverse)
      throw new IllegalStateException();

    if (forward)
      list.set(index - 1, e);
    if (reverse)
      list.set(index, e);
  }

}
