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
import python.lang.PyIterator;
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
 * These are the most generic type possible so that they can be used universally
 * throughout the bridge.
 */
public interface Backend
{

    PyByteArray bytearray(Object obj);

    PyByteArray bytearray_fromhex(String str);

    PyBytes bytes(Object obj);

    PyBytes bytes_fromhex(String str);

    PyComplex complex(double r, double i);

    void delattr(Object obj, String str);

    PyDict dict();

    PyList dir(Object obj);

    PyEnumerate enumerate(Object obj);

    PyObject eval(String source, PyDict globalsDict, PyDict localsDict);

    void exec(String source, PyDict globalsDict, PyDict localsDict);

    PyDict getDict(Object obj);

    PyObject getattr(Object obj, String str);

    public PyObject getitem(PyDict globalsDict, String key);

    boolean hasattr(Object obj, String str);

    boolean isinstance(Object obj, Object[] types);

    PyIterator iter(Object obj);

    <T> PyList list(Iterable<T> list);

    PyList list(Object... list);

    PyMemoryView memoryview(Object obj);

    PyDict newDict(Map<Object, Object> map);

    PyObject next(PyIterator iter, PyObject stop);

    PyObject object();

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
    PyRange range(int stop);

    PyRange range(int start, int stop);

    PyRange range(int start, int stop, int step);

    PyString repr(Object str);

    PySet set();

    void setattr(Object obj, String str, Object value);

    public void setitem(PyDict globalsDict, String key, Object value);

    PySlice slice(Integer start, Integer stop, int step);

    PyString str(Object str);

    PyIterator tee(PyIterator iter);

    <T> PyList tuple(Iterable<T> list);

    <T> PyTuple tuple(T... obj);

    PyType type(Object obj);

    <T> PyZip zip(T... objects);

}
