/* ****************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 * ***************************************************************************/
package org.jpype.bridge;

import java.util.Map;
import python.lang.PyBytes;
import python.lang.PyDict;
import python.lang.PyList;
import python.lang.PyMemoryView;
import python.lang.PyObject;
import python.lang.PyString;
import python.lang.PyTuple;
import python.lang.PyType;

/**
 * Backend for all Python entry points.
 *
 * This will be implemented by Python to allow access to Java. This class will
 * is not callable directly but is used by other support classes to provide
 * behavior.
 *
 */
public interface Backend
{

    PyType type(Object obj);

    PyList tuple(Iterable<Object> list);

    PyTuple tuple(Object... obj);

    PyDict newDict(Map<Object, Object> map);

    PyList list(Iterable<Object> list);

    PyList list(Object... list);

    PyString str(Object str);

    PyString repr(Object str);

    boolean hasattr(PyObject obj, String str);

    void delattr(PyObject obj, String str);

    PyObject getattr(PyObject obj, String str);

    void setattr(PyObject obj, String str, Object value);

    PyList dir(PyObject obj);

    PyDict getDict(PyObject obj);
    
    PyBytes bytes(PyObject obj);
    
    PyMemoryView memoryview(PyObject obj);
    

// It is okay if not everything is exposed as the user can always evaluate
// statements if something is missing.
//Creation
//   dict()
//   set()
//   tuple()
//   complex()
//   bytearray()
//   list()
//   memoryview()
//   open()
//   property()
//   slice()
//Casting? 
// These take one argument they act on so they could be considered object method
// place on PyOject?
//   ascii()
//   str()
//   int()
//   iter()
//   bytes()
//   classmethod()
//   float()
//   type()
// Int only methods?
//   bin()
//   hex()
//   oct()
// Methods?  move to PyObject
//   memoryview - asMemoryView()
//   bool() - asBool()
//   float() - asFloat()
//   int() - asInt()
//   callable() - isCallable
//   repr()
//   len()
//   id()
//   hash()
//   isinstance()
//   issubclass()
//   setattr()
//   getattr()
//   hasattr()
//   delattr()
//   dir()
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
// Iterator/generators
//   max()
//   min()
//   all()
//   sum()
//   map()
//   zip()
//   enumerate()
//   range()
//   reversed()
//   sorted()
//   any()
// Method for iterator
//  next()
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

    public boolean isinstance(Object obj, PyObject[] types);
}
