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
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;

/**
 * Java front end for concrete Python list.
 *
 * This mostly obeys the Java contract for List.
 */
public interface PyList extends List<PyObject>, PyIterable
{

  static PyType type()
  {
    return (PyType) BuiltIn.eval("list", null, null);
  }

  public static PyList create(Iterable c)
  {
    return BuiltIn.list(c);
  }

  @Override
  boolean add(PyObject e);

  @Override
  void add(int index, PyObject element);

  @Override
  default boolean addAll(Collection<? extends PyObject> c)
  {
    this.extend(c);
    return !c.isEmpty();
  }

  @Override
  default boolean addAll(int index, Collection<? extends PyObject> c)
  {
    this.insert(index, c);
    return !c.isEmpty();
  }

  void addAny(Object obj);

  @Override
  void clear();

  @Override
  boolean contains(Object o);

  @Override
  default boolean containsAll(Collection<?> c)
  {
    PySet s1 = PySet.create(this);
    PySet s2 = PySet.create(c);
    return s2.isSubset(s1);
  }

  void extend(Collection<? extends PyObject> c);

  @Override
  PyObject get(int index);

  @Override
  int indexOf(Object o);

  void insert(int index, Collection<? extends PyObject> c);

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
    return new PyListIterator(this, 0);
  }

  @Override
  default ListIterator<PyObject> listIterator(int index)
  {
    if (index < 0 || index > size())
      throw new IndexOutOfBoundsException();
    return new PyListIterator(this, index);
  }

  @Override
  default boolean remove(Object o)
  {
    return Interpreter.getBackend().delindex(this, this.indexOf(o));
  }

  @Override
  default PyObject remove(int index)
  {
    PyObject out = this.get(index);
    Interpreter.getBackend().delindex(this, index);
    return out;
  }

  @Override
  boolean removeAll(Collection<?> c);

  @Override
  boolean retainAll(Collection<?> c);

  @Override
  PyObject set(int index, PyObject element);

  void setAny(int index, Object obj);

  @Override
  int size();

  @Override
  PyList subList(int fromIndex, int toIndex);

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
