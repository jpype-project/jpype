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
import java.util.Set;

/**
 * Representation of a KeySet of Python Mappings.
 */
class PyMappingKeySet implements Set<Object>
{

  final PyMapping map;

  public PyMappingKeySet(PyMapping mapping)
  {
    this.map = mapping;
  }

  @Override
  public int size()
  {
    return map.size();
  }

  @Override
  public boolean isEmpty()
  {
    return map.isEmpty();
  }

  @Override
  public boolean contains(Object o)
  {
    return map.containsKey(o);
  }

  @Override
  public Iterator<Object> iterator()
  {
    // FIXME this one is required
    throw new UnsupportedOperationException("Not supported yet.");
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

  @Override
  public boolean add(Object e)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean remove(Object o)
  {
    return map.remove(o) != null;
  }

  @Override
  public boolean containsAll(Collection<?> c)
  {
    for (var x : c)
    {
      if (!map.containsKey(x))
        return false;
    }
    return true;
  }

  @Override
  public boolean addAll(Collection<? extends Object> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean removeAll(Collection<?> c)
  {
    boolean y = true;
    for (var x : c)
    {
      y &= this.remove(x);
    }
    return y;
  }

  @Override
  public void clear()
  {
    map.clear();
  }

}
