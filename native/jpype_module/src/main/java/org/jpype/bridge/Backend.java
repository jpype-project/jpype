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
package org.jpype.bridge;

import java.util.Map;
import python.lang.PyByteArray;
import python.lang.PyBytes;
import python.lang.PyComplex;
import python.lang.PyDict;
import python.lang.PyEnumerate;
import python.lang.PyList;
import python.lang.PyMemoryView;
import python.lang.PyObject;
import python.lang.PyRange;
import python.lang.PySet;
import python.lang.PySlice;
import python.lang.PyString;
import python.lang.PyTuple;
import python.lang.PyType;
import python.lang.PyZip;
import python.protocol.PyCallable;
import python.protocol.PyMapping;
import python.protocol.PyIter;

/**
 * Backend for all Python entry points.
 *
 * This will be implemented by Python to allow access to Java. This class will
 * is not callable directly but is used by other support classes to provide
 * behavior.
 *
 * The user should not call these directly. The user interface is located on
 * Context.
 *
 * Parameters are the most generic type possible so that they can be used
 * universally throughout the bridge. Return types are the most specific.
 *
 * CharSequence will be used rather than the usual Java String as it allows both
 * Python str and Java String through the same interface.
 */
public interface Backend
{

  PyByteArray bytearray(Object obj);

  PyByteArray bytearray_fromhex(CharSequence str);

  PyBytes bytes(Object obj);

  PyBytes bytes_fromhex(CharSequence str);

  PyObject call(PyCallable obj, PyTuple args, PyDict kwargs);

  PyComplex complex(double r, double i);

  void delattr(Object obj, CharSequence str);

  public boolean delindex(PyList aThis, int indexOf);

  PyDict dict();

  PyList dir(Object obj);

  PyEnumerate enumerate(Object obj);

  PyObject eval(CharSequence source, PyDict globalsDict, PyMapping localsDict);

  void exec(CharSequence source, PyDict globalsDict, PyMapping localsDict);

  PyDict getDict(Object obj);

  PyObject getattr(Object obj, CharSequence str);

  public PyObject getitem(PyDict globalsDict, CharSequence key);

  boolean hasattr(Object obj, CharSequence str);

  boolean isinstance(Object obj, Object[] types);

  PyIter iter(Object obj);

  <T> PyList list(Iterable<T> list);

  PyList list(Object... list);

  PyMemoryView memoryview(Object obj);

  PyDict newDict(Map<Object, Object> map);

  public PySet newSet(Iterable c);

  PyObject next(PyIter iter, PyObject stop);

  PyObject object();

  PyRange range(int stop);

  PyRange range(int start, int stop);

  PyRange range(int start, int stop, int step);

  PyString repr(Object str);

  PySet set();

  void setattr(Object obj, CharSequence str, Object value);

  public void setitem(PyDict globalsDict, CharSequence key, Object value);

  PySlice slice(Integer start, Integer stop, Integer step);

  PyString str(Object str);

  PyIter tee(PyIter iter);

  <T> PyList tuple(Iterable<T> list);

  <T> PyTuple tuple(T... obj);

  PyType type(Object obj);

  PyObject items(PyObject obj);

  PyObject values(PyObject obj);

  <T> PyZip zip(T... objects);
// It is okay if not everything is exposed as the user can always evaluate
// statements if something is missing.
//Creation
//   open()
//   property()
//Casting? 
// These take one argument they act on so they could be considered object method
// place on PyOject?
//   ascii()
//   int()
//   classmethod()
//   float()
// Int only methods?
//   bin()
//   hex()
//   oct()
// Methods?  move to PyObject
//   id()
//   issubclass()
//   vars()
// Belong on scopes?
//   compile()
//   eval()
//   exec()
//   globals()
//   locals()
// Math
//   abs()
//   pow()
//   divmod()
//   chr()
//   ord()
//   round()
// Method for iterator
//  filter()
//aiter()
//anext()
//breakpoint()
//format()
//frozenset()
//help()
//input()
//object()
//print()
//staticmethod()
//super()

}
