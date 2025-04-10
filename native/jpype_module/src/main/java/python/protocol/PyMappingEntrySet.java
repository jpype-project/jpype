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
import java.util.Map;
import java.util.Set;
import org.jpype.bridge.BuiltIn;
import python.lang.PyObject;

/**
 *
 * @author nelson85
 */
public class PyMappingEntrySet implements Set<Map.Entry<Object, PyObject>>
{

  private final PyObject items;
  private PyMapping map;

  PyMappingEntrySet(PyMapping map, PyObject items)
  {
    this.map = map;
    this.items = items;
  }

  @Override
  public boolean add(Map.Entry<Object, PyObject> e)
  {
    this.map.put(e.getKey(), e.getValue());
    return true;
  }

  @Override
  public boolean addAll(Collection<? extends Map.Entry<Object, PyObject>> c)
  {
    for (var v : c)
      this.add(v);
    return true;
  }

  @Override
  public void clear()
  {
    this.map.clear();
  }

  @Override
  public boolean contains(Object o)
  {
    return false;
  }

  @Override
  public boolean containsAll(Collection<?> c)
  {
    return false;
  }

  @Override
  public boolean isEmpty()
  {
    return map.isEmpty();
  }

  @Override
  public Iterator<Map.Entry<Object, PyObject>> iterator()
  {
    return new PyMappingEntrySetIterator(map, BuiltIn.iter(this.items));
  }

  @Override
  public boolean remove(Object o)
  {
    return false;
  }

  @Override
  public boolean removeAll(Collection<?> c)
  {
    return false;
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
