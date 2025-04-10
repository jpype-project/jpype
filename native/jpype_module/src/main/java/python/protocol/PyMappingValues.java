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
import org.jpype.bridge.BuiltIn;
import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
class PyMappingValues implements Collection<PyObject>
{

  private final PyMapping map;
  private final PyObject values;

  public PyMappingValues(PyMapping map, PyObject values)
  {
    this.map = map;
    this.values = values;
  }

  @Override
  public boolean add(PyObject e)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean addAll(Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public void clear()
  {
    this.map.clear();
  }

  @Override
  public boolean contains(Object o)
  {
    return this.map.containsValue(o);
  }

  @Override
  public boolean containsAll(Collection<?> c)
  {
    for (var v : c)
    {
      if (!this.map.containsValue(v))
        return false;
    }
    return true;
  }

  @Override
  public boolean isEmpty()
  {
    return this.map.isEmpty();
  }

  @Override
  public Iterator<PyObject> iterator()
  {
    return BuiltIn.iter(this.values).iterator();
  }

  @Override
  public boolean remove(Object o)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean removeAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public int size()
  {
    return this.map.size();
  }

  @Override
  public Object[] toArray()
  {
    return new ArrayList(this).toArray();
  }

  @Override
  public <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList(this).toArray(a);
  }

}
