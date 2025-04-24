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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.Set;
import org.jpype.bridge.Interpreter;
import org.jpype.bridge.BuiltIn;

/**
 * Java front end for concrete Python set.
 *
 * This mostly obeys the Java contract for sets.
 *
 */
public interface PyFrozenSet extends PyObject, Set<PyObject>
{

  static PyType type()
  {
    return (PyType) BuiltIn.eval("set", null, null);
  }

  /**
   * Create a Python set from an Iterable.
   *
   * @param c
   * @return
   */
  public static PyFrozenSet of(Iterable c)
  {
    return Interpreter.getBackend().newFrozenSet(c);
  }

  @Override
  public boolean add(PyObject e);

  @Override
  default public boolean addAll(Collection<? extends PyObject> c)
  {
    int l1 = this.size();
    this.update(of(c));
    int l2 = this.size();
    return l1 != l2;
  }

  @Override
  void clear();

  @Override
  boolean contains(Object o);

  @Override
  default public boolean containsAll(Collection<?> c)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Shallow copy.
   *
   * @return a new set
   */
  PyFrozenSet copy();

  PyFrozenSet difference(PyFrozenSet... set);

  void discard(Object item);

  PyFrozenSet intersect(PyFrozenSet... set);

  boolean isDisjoint(PyFrozenSet set);

  @Override
  default public boolean isEmpty()
  {
    return size() == 0;
  }

  boolean isSubset(PyFrozenSet set);

  boolean isSuperset(PyFrozenSet set);

  @Override
  default Iterator<PyObject> iterator()
  {
    return Interpreter.getBackend().iter(this).iterator();
  }

  PyObject pop();

  @Override
  default public boolean removeAll(Collection<?> c)
  {
    int l1 = this.size();
    PyObject delta = this.difference(of(c));
    this.clear();
    this.update(delta);
    int l2 = this.size();
    return l1 != l2;
  }

  @Override
  default public boolean retainAll(Collection<?> c)
  {
    int l1 = this.size();
    PyObject delta = this.intersect(of(c));
    this.clear();
    this.update(delta);
    int l2 = this.size();
    return l1 != l2;
  }

  @Override
  public int size();

  PyFrozenSet symmetricDifference(PyFrozenSet... set);

  @Override
  default public Object[] toArray()
  {
    return new ArrayList(this).toArray();
  }

  @Override
  default public <T> T[] toArray(T[] a)
  {
    return (T[]) new ArrayList(this).toArray(a);
  }

  PyFrozenSet union(PyFrozenSet... set);

  void update(PyObject other);

}
