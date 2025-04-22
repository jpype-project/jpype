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

import python.protocol.PyIterable;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import org.jpype.bridge.BuiltIn;

/**
 * Java front end for concrete Python tuple.
 *
 * FIXME add the List contract.
 */
public interface PyTuple extends PyIterable, List<PyObject>
{

  public static PyTuple create(Object... values)
  {
    return BuiltIn.tuple(values);
  }

  static PyType type()
  {
    return (PyType) BuiltIn.eval("tuple", null, null);
  }

  @Override
  default boolean add(PyObject e)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default void add(int index, PyObject element)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default boolean addAll(Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default boolean addAll(int index, Collection<? extends PyObject> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default void clear()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public boolean contains(Object o);

  @Override
  default boolean containsAll(Collection<?> c)
  {
    PySet s1 = PySet.create(this);
    PySet s2 = PySet.create(c);
    return s2.isSubset(s1);
  }

  @Override
  PyObject get(int index);

  @Override
  public int indexOf(Object o);

  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  @Override
  default Iterator<PyObject> iterator()
  {
    return new PyIterator(this.iter());
  }

  @Override
  default int lastIndexOf(Object o)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default ListIterator<PyObject> listIterator()
  {
    return new PyTupleIterator(this, 0);
  }

  @Override
  default ListIterator<PyObject> listIterator(int index)
  {
    if (index < 0 || index > size())
      throw new IndexOutOfBoundsException();
    return new PyTupleIterator(this, index);
  }

  @Override
  default boolean remove(Object o)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default PyObject remove(int index)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default boolean removeAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  default PyObject set(int index, PyObject element)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  int size();

  @Override
  PyTuple subList(int fromIndex, int toIndex);

  @Override
  default Object[] toArray()
  {
    return new ArrayList(this).toArray();
  }

  @Override
  default <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList(this).toArray(a);
  }

}
