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

import java.util.Collection;
import java.util.Map;
import java.util.concurrent.Future;
import python.lang.PyByteArray;
import python.lang.PyBytes;
import python.lang.PyComplex;
import python.lang.PyDict;
import python.lang.PyEnumerate;
import python.lang.PyFloat;
import python.lang.PyFrozenSet;
import python.lang.PyInt;
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
import python.protocol.PyBuffer;
import python.protocol.PyCallable;
import python.protocol.PyMapping;
import python.protocol.PyIter;
import python.protocol.PyIterable;
import python.protocol.PyMappingValues;

/**
 * Backend for all Python entry points.
 *
 * This will be implemented by Python to allow access to Java. This class will
 * is not callable directly but is used by other support classes to provide
 * behavior.
 *
 * The user should not call these directly. The user interface is located on
 * BuiltIn and Context.
 *
 * Parameters are the most generic type possible so that they can be used
 * universally throughout the bridge. Return types are the most specific.
 *
 * CharSequence will be used rather than the usual Java String as it allows both
 * Python str and Java String through the same interface.
 *
 * Generally these methods should not be overloaded as we may need use different
 * implementations depending on the arguments.
 */
public interface Backend
{

  // Create a Python `bytearray` from the given object.
  PyByteArray bytearray(Object obj);

  // Create a Python `bytearray` from a hex-encoded string.
  PyByteArray bytearrayFromHex(CharSequence str);

  // Create a Python `bytes` object from the given object.
  PyBytes bytes(Object obj);

  // Create a Python `bytes` object from a hex-encoded string.
  PyBytes bytesFromHex(CharSequence str);

  // Call a Python callable with arguments and keyword arguments.
  PyObject call(PyCallable obj, PyTuple args, PyDict kwargs);

  // Call a Python callable asynchronously with arguments and keyword arguments.
  Future<PyObject> callAsync(PyCallable aThis, PyTuple args, PyDict kwargs);

  // Call a Python callable asynchronously with a timeout.
  Future<PyObject> callAsyncWithTimeout(PyCallable aThis, PyTuple args, PyDict kwargs, long timeout);

  // Delete an attribute from a Python object.
  void delattrString(Object obj, CharSequence arg);

  // Delete an item from a Python object by index.
  boolean delByIndex(Object obj, int index);

  // Get a list of attributes for a Python object (`dir`).
  PyList dir(Object obj);

  // Create a Python `enumerate` object from the given iterable.
  PyEnumerate enumerate(Object obj);

  // Evaluate a Python expression with optional globals and locals.
  PyObject eval(CharSequence source, PyDict globals, PyMapping locals);

  // Execute a Python statement with optional globals and locals.
  void exec(CharSequence source, PyDict globals, PyMapping locals);

  // Get the type of a Python callable as a string.
  String getCallableType(PyCallable obj);

  // Get the `__dict__` attribute of a Python object.
  PyDict getDict(Object obj);

  // Get the docstring of a Python callable.
  String getDocString(PyCallable obj);

  // Get the signature of a Python callable.
  PyObject getSignature(PyCallable obj);

  public PyObject getattrObject(PyObject obj, Object key);

  // Get an attribute from a Python object by name.
  PyObject getattrString(Object obj, CharSequence str);

  // Get an item from a Python mapping object by key.
  PyObject getitemMappingObject(PyObject obj, PyObject key);

  // Get an item from a Python mapping object by string key.
  PyObject getitemMappingString(PyObject obj, CharSequence key);

  // Get an item from a Python sequence by index.
  PyObject getitemSequence(PyObject obj, int index);

  // Check if a Python object has a specific attribute.
  boolean hasattrString(Object obj, CharSequence key);

  // Check if a Python callable is callable.
  public boolean isCallable(PyCallable obj);

  // Check if a Python object is an instance of any of the specified types.
  boolean isinstanceFromArray(Object obj, Object[] types);

  // Get the items of a Python mapping object.
  PyObject items(PyObject obj);

  // Create a Python iterator from the given object.
  PyIter iter(Object obj);

  // Get the length of a Python object (`len`).
  public int len(Object aThis);

  public void mappingClear();

  public void mappingClear(PyMapping aThis);

  public boolean mappingContainsAllValues(PyMappingValues aThis, Collection<?> c);

  public boolean mappingContainsAllValues(PyMapping map, Collection<?> c);

  public boolean mappingContainsValue(Object value);

  public boolean mappingContainsValue(PyMapping aThis, Object value);

  public boolean mappingRemoveAllKeys(PyMapping map, Collection<?> collection);

  public boolean mappingRemoveAllValue(PyMapping map, Collection<?> collection);

  public boolean mappingRemoveValue(PyMapping map, Object value);

  public boolean mappingRetainAllKeys(PyMapping map, Collection<?> collection);

  public boolean mappingRetainAllValue(PyMapping map, Collection<?> collection);

  // Create a Python `memoryview` from the given object.
  PyMemoryView memoryview(Object obj);

  // Create an empty Python `bytearray`.
  PyByteArray newByteArray();

  // Create a Python `bytearray` from a buffer.
  PyByteArray newByteArrayFromBuffer(PyBuffer bytes);

  // Create a Python `bytearray` from an iterable.
  PyByteArray newByteArrayFromIterable(Iterable iter);

  // Create a Python `bytearray` with a specified size.
  PyByteArray newByteArrayOfSize(int i);

  // Create a Python `bytes` object from a buffer.
  PyByteArray newBytesFromBuffer(PyBuffer bytes);

  // Create a Python `bytes` object from an iterator.
  PyByteArray newBytesFromIterator(Iterable<PyObject> iter);

  // Create a Python `bytes` object with a specified size.
  PyBytes newBytesOfSize(int length);

  // Create a Python `complex` number with real and imaginary parts.
  PyComplex newComplex(double real, double imag);

  // Create an empty Python `dict`.
  PyDict newDict();

  // Create a Python `dict` from a Java `Map`.
  PyDict newDict(Map map);

  // Create a Python `dict` from an iterable of key-value pairs.
  public PyDict newDictFromIterable(Iterable map);

  // Create a Python `enumerate` object from an iterable.
  PyEnumerate newEnumerate(Iterable iterable);

  // Create a Python `float` from a double value.
  PyFloat newFloat(double value);

  // Create a Python `frozenset` from an iterable.
  PyFrozenSet newFrozenSet(Iterable c);

  // Create a Python `int` from a long value.
  PyInt newInt(long value);

  // Create a Python `list` from an array of objects.
  PyList newListFromArray(Object... list);

  // Create a Python `list` from an iterable.
  <T> PyList newListFromIterable(Iterable<T> list);

  // Create a Python `set` from an iterable.
  PySet newSet(Iterable c);

  // Create a Python `tuple` from an array of objects.
  <T> PyTuple newTupleFromArray(T... obj);

  // Create a Python `tuple` from an iterable.
  <T> PyList newTupleFromIterator(Iterable<T> list);

  // Create a Python `zip` object from multiple iterables.
  PyFloat newZip(PyIterable[] items);

  // Get the next item from a Python iterator, with a stop value.
  PyObject next(Object iter, Object stop);

  // Create a generic Python object.
  PyObject object();

  // Create a Python `range` object with a stop value.
  PyRange range(int stop);

  // Create a Python `range` object with start and stop values.
  PyRange range(int start, int stop);

  // Create a Python `range` object with start, stop, and step values.
  PyRange range(int start, int stop, int step);

  // Get the string representation of an object (`repr`).
  PyString repr(Object str);

  // Create an empty Python `set`.
  PySet set();

  // Perform a union operation on two Python sets.
  PySet setUnionIterable(PySet set1, PySet set2);

  // Perform a union operation on multiple Python sets.
  PySet setUnionMultiple(PySet set1, PySet[] sets);

  // Set an attribute on a Python object.
  void setattr(Object obj, CharSequence str, Object value);

  public PyObject setattrReturn(PyObject obj, CharSequence key, PyObject value);

  // Set an item in a Python dictionary.
  void setitem(PyDict globalsDict, CharSequence key, Object value);

  // Create a Python `slice` object.
  PySlice slice(Integer start, Integer stop, Integer step);

  // Convert an object to a Python `str`.
  PyString str(Object str);

  // Create a Python `tee` iterator.
  PyIter teeIterator(PyIter iter);

  // Get the type of a Python object.
  PyType type(Object obj);

  // Get the values of a Python mapping object.
  PyObject values(Object obj);

  public PyDict vars(Object obj);

  // Create a Python `zip` object from multiple objects.
  <T> PyZip zip(T... objects);
}
