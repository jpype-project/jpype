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
import java.util.Set;
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
import python.protocol.PyIndex;
import python.protocol.PyMapping;
import python.protocol.PyIter;

/**
 * Backend for all Python entry points.
 *
 * This interface is implemented by Python to allow access to Java. It is not
 * callable directly but serves as a bridge used by other support classes to
 * provide functionality.
 *
 * The user should not interact with this interface directly. Instead, the
 * user-facing API is available through `BuiltIn` and `Context`.
 *
 * Parameters are kept as generic as possible to ensure universal applicability
 * across the Python-Java bridge. Return types are chosen to be the most
 * specific to match Python's behavior.
 *
 * `CharSequence` is preferred over `String` to accommodate both Python `str`
 * and Java `String` seamlessly.
 *
 * Avoid overloading these methods unnecessarily, as different implementations
 * may be required depending on the arguments.
 */
public interface Backend
{

  // Create a Python `bytearray` from the specified object.
  PyByteArray bytearray(Object obj);

  // Create a Python `bytearray` from a hex-encoded string.
  PyByteArray bytearrayFromHex(CharSequence str);

  // Create a Python `bytes` object from the specified object.
  PyBytes bytes(Object obj);

  // Create a Python `bytes` object from a hex-encoded string.
  PyBytes bytesFromHex(CharSequence str);

  // Call a Python callable with positional and keyword arguments.
  PyObject call(PyCallable obj, PyTuple args, PyDict kwargs);

  // Call a Python callable asynchronously with positional and keyword arguments.
  Future<PyObject> callAsync(PyCallable callable, PyTuple args, PyDict kwargs);

  // Call a Python callable asynchronously with a timeout.
  Future<PyObject> callAsyncWithTimeout(PyCallable callable, PyTuple args, PyDict kwargs, long timeout);

  // Check if the specified value is contained within the given Python object.
  boolean contains(Object obj, Object value);

  // Delete an item from a Python sequence by index.
  boolean delByIndex(Object obj, int index);

  // Delete an item from a Python mapping object by key.
  PyObject delByObject(PyMapping obj, Object key);

  PyObject delattrReturn(PyObject obj, Object key);

  // Delete an attribute from a Python object by name.
  void delattrString(Object obj, CharSequence attrName);

  // Get a list of attributes for a Python object (equivalent to Python's `dir()`).
  PyList dir(Object obj);

  // Create a Python `enumerate` object from the specified iterable.
  PyEnumerate enumerate(Object obj);

  // Evaluate a Python expression with optional global and local variables.
  PyObject eval(CharSequence source, PyDict globals, PyObject locals);

  // Execute a Python statement with optional global and local variables.
  void exec(CharSequence source, PyDict globals, PyObject locals);

  // Get the type of a Python callable as a string.
  String getCallableType(PyCallable obj);

  // Get the `__dict__` attribute of a Python object.
  PyDict getDict(Object obj);

  // Get the docstring of a Python callable.
  String getDocString(PyCallable obj);

  // Get the signature of a Python callable.
  PyObject getSignature(PyCallable obj);

  PyObject getattrDefault(PyObject obj, Object key, PyObject defaultValue);

  // Get an attribute from a Python object by key.
  PyObject getattrObject(PyObject obj, Object key);

  // Get an attribute from a Python object by name.
  PyObject getattrString(Object obj, CharSequence attrName);

  // Get an item from a Python mapping object by key.
  PyObject getitemMappingObject(Object obj, Object key);

  // Get an item from a Python mapping object by string key.
  PyObject getitemMappingString(Object obj, CharSequence key);

  // Get an item from a Python sequence by index.
  PyObject getitemSequence(Object obj, int index);

  // Check if a Python object has a specific attribute.
  boolean hasattrString(Object obj, CharSequence attrName);

  // Check if the specified object is callable in Python.
  boolean isCallable(Object obj);

  // Check if a Python object is an instance of any of the specified types.
  boolean isinstanceFromArray(Object obj, Object[] types);

  // Get the items of a Python mapping object.
  PyObject items(PyObject obj);

  // Create a Python iterator from the specified object.
  <T> PyIter<T> iter(Object obj);

  <T> PyIter<T> iterSet(Set<T> obj);

  <T, V> PyIter<T> iterMap(Map<T, V> obj);

  // Get the keys of a Python mapping object.
  PyObject keys(Object dict);

  // Get the length of a Python object (equivalent to Python's `len()`).
  int len(Object obj);

  // Create a Python `list` from the specified object.
  PyList list(Object obj);

  // Clear all items in a Python mapping object.
  void mappingClear();

  // Clear all items in the specified Python mapping object.
  void mappingClear(Object obj);

  // Check if all values in the collection are contained in the Python mapping object.
  boolean mappingContainsAllValues(Object map, Collection<?> c);

  // Check if the specified value is contained in the Python mapping object.
  boolean mappingContainsValue(Object map, Object value);

  // Remove all keys in the collection from the Python mapping object.
  boolean mappingRemoveAllKeys(Object map, Collection<?> collection);

  // Remove all values in the collection from the Python mapping object.
  boolean mappingRemoveAllValue(Object map, Collection<?> collection);

  // Remove the specified value from the Python mapping object.
  boolean mappingRemoveValue(Object map, Object value);

  // Retain only the specified keys in the Python mapping object.
  boolean mappingRetainAllKeys(Object map, Collection<?> collection);

  // Retain only the specified values in the Python mapping object.
  boolean mappingRetainAllValue(Object map, Collection<?> collection);

  // Create a Python `memoryview` from the specified object.
  PyMemoryView memoryview(Object obj);

  // Create an empty Python `bytearray`.
  PyByteArray newByteArray();

  // Create a Python `bytearray` from a buffer.
  PyByteArray newByteArrayFromBuffer(PyBuffer bytes);

  // Create a Python `bytearray` from an iterable.
  PyByteArray newByteArrayFromIterable(Iterable<?> iter);

  public PyByteArray newByteArrayFromIterator(Iterable<PyObject> iterable);

  // Create a Python `bytearray` with a specified size.
  PyByteArray newByteArrayOfSize(int size);

  // Create a Python `bytes` object from a buffer.
  PyBytes newBytesFromBuffer(PyBuffer bytes);

  // Create a Python `bytes` object from an iterator.
  PyBytes newBytesFromIterator(Iterable<PyObject> iter);

  // Create a Python `bytes` object with a specified size.
  PyBytes newBytesOfSize(int size);

  // Create a Python `complex` number with real and imaginary parts.
  PyComplex newComplex(double real, double imag);

  // Create an empty Python `dict`.
  PyDict newDict();

  // Create a Python `dict` from a Java `Map`.
  PyDict newDict(Map<?, ?> map);

  // Create a Python `dict` from an iterable of key-value pairs.
  PyDict newDictFromIterable(Iterable<?> iterable);

  // Create a Python `enumerate` object from an iterable.
  PyEnumerate newEnumerate(Iterable<?> iterable);

  // Create a Python `float` from a double value.
  PyFloat newFloat(double value);

  // Create a Python `frozenset` from an iterable.
  PyFrozenSet newFrozenSet(Iterable<?> iterable);

  // Create a Python `int` from a long value.
  PyInt newInt(long value);

  // Create an empty Python `list`.
  PyList newList();

  // Create a Python `list` from an array of objects.
  PyList newListFromArray(Object... elements);

  // Create a Python `list` from an iterable.
  <T> PyList newListFromIterable(Iterable<T> iterable);

  // Create an empty Python `set`.
  PySet newSet();

  // Create a Python `set` from an iterable.
  PySet newSetFromIterable(Iterable<?> iterable);

  PyTuple newTuple();

  // Create a Python `tuple` from an array of objects.
  PyTuple newTupleFromArray(Object... elements);

  // Create a Python `tuple` from an iterable.
  PyTuple newTupleFromIterator(Iterable<?> iterable);

  // Create a Python `zip` object from multiple iterables.
  PyZip newZip(Object[] iterables);

  // Get the next item from a Python iterator, with a stop value.
  PyObject next(Object iterator, Object stop);

  // Create a generic Python object.
  PyObject object();

  // Create a Python `range` object with a stop value.
  PyRange range(int stop);

  // Create a Python `range` object with start and stop values.
  PyRange range(int start, int stop);

  // Create a Python `range` object with start, stop, and step values.
  PyRange range(int start, int stop, int step);

  // Get the string representation of an object (equivalent to Python's `repr()`).
  PyString repr(Object obj);

  // Create an empty Python `set`.
  PySet set();

  // Set an attribute on a Python object and return the updated object.
  PyObject setattrReturn(PyObject obj, Object attrName, Object value);

  // Set an attribute on a Python object.
  void setattrString(Object obj, CharSequence attrName, Object value);

  // Set an item in a Python mapping object by object key.
  PyObject setitemFromObject(Object obj, Object key, Object value);

  // Set an item in a Python mapping object by string key.
  void setitemFromString(Object obj, CharSequence key, Object value);

  void setitemMapping(Object obj, Object index, Object values);

  PyObject setitemSequence(Object obj, int index, Object value);

  // Create a Python `slice` object.
  PySlice slice(Integer start, Integer stop, Integer step);

  // Convert an object to a Python `str`.
  PyString str(Object obj);

  // Create a Python `tee` iterator.
  PyIter teeIterator(PyIter iterator);

  // Get the type of a Python object.
  PyType type(Object obj);

  // Get the values of a Python mapping object.
  PyObject values(Object obj);

  // Get the `__dict__` attribute of a Python object.
  PyDict vars(Object obj);

  PyZip zipFromArray(Object[] objects);

  // Create a Python `zip` object from multiple objects.
  PyZip zipFromIterable(Object... objects);
}
