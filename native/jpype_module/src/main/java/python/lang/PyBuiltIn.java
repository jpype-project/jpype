// --- file: python/lang/PyBuiltIn.java ---
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

import java.util.Arrays;
import java.util.Map;
import org.jpype.Backend;
import org.jpype.internal.NativeContext;

/**
 * Utility class providing built-in functions similar to Python's built-in
 * functions.
 *
 * In general these are set as widely as possible. Many will accept Java objects
 */
public class PyBuiltIn
{

  protected final Backend backend;
  protected final NativeContext context;

  public Backend getBackend()
  {
    return this.backend;
  }

  /**
   * Returns the {@link NativeContext} for this interpreter — the same
   * per-interpreter object reachable from any live {@link PyObject} via
   * {@link PyObject#builtin()}, rather than a JVM-wide static.
   *
   * @return The {@link NativeContext} instance.
   */
  public NativeContext getContext()
  {
    return this.context;
  }

  /**
   * Looks up an SPI-provided mini-backend for this interpreter (e.g.
   * {@code python.io.IO}), reached through this same per-interpreter
   * {@code PyBuiltIn} instance rather than any JVM-wide static — any live
   * {@link PyObject} can reach this via {@code obj.builtin().getBackend(...)}.
   *
   * @param <T> the mini-backend interface type.
   * @param iface the mini-backend interface.
   * @return the registered instance.
   * @throws IllegalStateException if nothing is registered for {@code iface}.
   */
  public <T> T getBackend(Class<T> iface)
  {
    return context.getBackend(iface);
  }

  protected PyBuiltIn(NativeContext context, Backend backend)
  {
    this.context = context;
    this.backend = backend;
  }

  /**
   * Creates a new Python float object.
   *
   * @param value the double value to be converted to a Python float.
   * @return a new {@link PyFloat} instance representing the given value.
   */
  public PyFloat $float(double value)
  {
    return backend.newFloat(value);
  }

  /**
   * Creates a new Python integer object.
   *
   * @param value the long value to be converted to a Python integer.
   * @return a new {@link PyInt} instance representing the given value.
   */
  public PyInt $int(long value)
  {
    return backend.newInt(value);
  }

  public double asDouble(PyObject obj)
  {
    return backend.asDouble(obj);
  }

  public long asLong(PyObject obj)
  {
    return backend.asLong(obj);
  }

  /**
   * Creates a new Python bytes object from the given input.
   *
   * @param obj the object to be converted to a bytes representation.
   * @return a new {@link PyBytes} instance representing the bytes of the
   * object.
   */
  public PyBytes bytes(Object obj)
  {
    return backend.bytes(obj);
  }

  public PyByteArray bytearray(int size)
  {
    return backend.bytearray(size);
  }

  /**
   * Creates a new Python bytearray from an iterable of Python objects. Each
   * object in the iterable must be convertible to a byte.
   *
   * @param iter the iterable containing {@link PyObject} instances to include
   * in the bytearray.
   * @return a new {@link PyByteArray} instance containing the bytes derived
   * from the iterable.
   */
  public PyByteArray bytearray(Iterable<PyObject> iter)
  {
    return backend.newByteArrayFromIterable(iter);
  }

  /**
   * Creates a new Python bytearray from a {@link PyBuffer}. The buffer's
   * contents will be used to initialize the bytearray.
   *
   * @param bytes the {@link PyBuffer} containing the data to populate the
   * bytearray.
   * @return a new {@link PyByteArray} instance containing the bytes from the
   * buffer.
   */
  public PyByteArray bytearray(PyBuffer bytes)
  {
    return backend.newByteArrayFromBuffer(bytes);
  }

  /**
   * Creates a new Python bytearray from a hexadecimal string. The input string
   * must contain valid hexadecimal characters.
   *
   * @param str the hexadecimal string to convert into a bytearray.
   * @return a new {@link PyByteArray} instance containing the bytes represented
   * by the hex string.
   */
  public PyByteArray bytearrayFromHex(CharSequence str)
  {
    return backend.bytearrayFromHex(str);
  }

  /**
   * Creates a new Python `bytes` object with a fixed length. The `bytes` object
   * will be initialized with zero bytes.
   *
   * @param length the length of the `bytes` object to create.
   * @return a new {@link PyBytes} instance with the specified length.
   */
  public PyBytes bytes(int length)
  {
    return backend.newBytesOfSize(length);
  }

  /**
   * Decodes the contents of the `bytes` object using the specified encoding.
   *
   * Optionally, specific bytes can be deleted during decoding.
   *
   * @param encoding the encoding to use for decoding (e.g., "utf-8").
   * @param delete the bytes to delete during decoding, or {@code null} for no
   * deletion.
   * @return a {@link PyObject} representing the decoded string.
   */
  public PyBytes bytesFromHex(CharSequence str)
  {
    return backend.bytesFromHex(str);
  }
//
//  /**
//   * Creates a new Python `bytes` object from an iterable of Python objects.
//   *
//   * Each object in the iterable must be convertible to a byte.
//   *
//   * @param iterable the iterable containing {@link PyObject} instances to
//   * include in the `bytes` object.
//   * @return a new {@link PyBytes} instance containing the bytes derived from
//   * the iterable.
//   */
//  public PyBytes bytes(Iterable<PyObject> iterable)
//  {
//    return backend.newBytesFromIterator(iterable);
//  }

  /**
   * Creates a new Python `bytes` object from a {@link PyBuffer}.
   *
   * The buffer's contents will be used to initialize the `bytes` object.
   *
   * @param buffer the {@link PyBuffer} containing the data to populate the
   * `bytes` object.
   * @return a new {@link PyBytes} instance containing the bytes from the
   * buffer.
   */
  public PyBytes bytes(PyBuffer buffer)
  {
    return backend.newBytesFromBuffer(buffer);
  }

  /**
   * Invoke a Python callable object with the specified arguments and keyword
   * arguments.
   *
   * @param obj the callable object to invoke.
   * @param args the positional arguments for the callable.
   * @param kwargs the keyword arguments for the callable.
   * @return the result of the callable execution as a {@link PyObject}.
   */
  public PyObject call(PyCallable obj, PyTuple args, PyDict kwargs)
  {
    return backend.call(obj, args, kwargs);
  }

  /**
   * Creates a new Python `complex` number with the specified real and imaginary
   * parts.
   *
   * @param real the real part of the complex number.
   * @param imag the imaginary part of the complex number.
   * @return a new {@link PyComplex} instance representing the complex number.
   */
  public PyComplex complex(double real, double imag)
  {
    return backend.newComplex(real, imag);
  }

  /**
   * Deletes an attribute from a Python object.
   *
   * @param obj the Python object from which the attribute will be removed.
   * @param key the name of the attribute to delete.
   */
  public void delattr(PyObject obj, CharSequence key)
  {
    backend.delattrString(obj, key);
  }

  public PyDict dict()
  {
    return backend.newDict();
  }

  public PyDict dictFromItems(Iterable<? extends Map.Entry<?, ?>> map)
  {
    return backend.newDictFromIterable(map);
  }

  /**
   * Creates a new Python `dict` object from the specified Java {@link Map}.
   *
   * The keys in the provided map are converted to Python objects, and the
   * values must already be instances of {@link PyObject}.
   *
   * @param map the Java {@link Map} whose entries will populate the Python
   * `dict`. Keys are converted to Python objects, and values are expected to be
   * {@link PyObject}.
   * @return a new {@link PyDict} instance representing the Python dictionary.
   */
  public PyDict dictFromMap(Map<? extends Object, ? extends Object> map)
  {
    return backend.newDict(map);
  }

  /**
   * Returns a list of attribute names for a Python object.
   *
   * @param obj the Python object to inspect.
   * @return a {@link PyList} containing the attribute names.
   */
  public PyList dir(PyObject obj)
  {
    return backend.dir(obj);
  }

  /**
   * Creates a Python enumerate object from the given iterable.
   *
   * @param obj the iterable to enumerate.
   * @return a new {@link PyEnumerate} instance.
   */
  public PyEnumerate enumerate(PyObject obj)
  {
    return backend.enumerate(obj);
  }

  /**
   * Creates a Python enumerate object from the given Java iterable.
   *
   * @param obj the Java iterable to enumerate.
   * @return a new {@link PyEnumerate} instance.
   */
  public PyEnumerate enumerate(Iterable<?> obj)
  {
    return backend.enumerate(obj);
  }

  /**
   * Evaluates a Python expression in the given global and local namespaces.
   *
   * @param statement the Python expression to evaluate.
   * @param globals the global namespace as a {@link PyDict}.
   * @param locals the local namespace as a {@link PyObject}.
   * @return the result of the evaluation as a {@link PyObject}.
   */
  public PyObject eval(CharSequence statement, PyDict globals, PyObject locals)
  {
    return backend.eval(statement, globals, locals);
  }

  /**
   * Executes a Python statement in the given global and local namespaces.
   *
   * @param statement the Python statement to execute.
   * @param globals the global namespace as a {@link PyDict}.
   * @param locals the local namespace as a {@link PyMapping}.
   */
  public void exec(CharSequence statement, PyDict globals, PyMapping<?, ?> locals)
  {
    backend.exec(statement, globals, locals);
  }

  /**
   * Retrieves the value of an attribute from a Python object.
   *
   * @param obj the Python object to inspect.
   * @param key the name of the attribute to retrieve.
   * @return the value of the attribute as a {@link PyObject}.
   */
  public PyObject getattr(PyObject obj, PyString key)
  {
    return backend.getattr(obj, key);
  }

  public PyFrozenSet frozenset(Object c)
  {
    return backend.frozenset(c);
  }

  /**
   * Creates a new Python `frozenset` object from the specified
   * {@link Iterable}.
   *
   * @param c the {@link Iterable} whose elements will be included in the
   * `frozenset`.
   * @return a new {@link PyFrozenSet} instance representing the Python
   * `frozenset` object.
   */
  public PyFrozenSet frozensetFromIterable(Iterable<?> c)
  {
    return backend.newFrozenSet(c);
  }

  public PyObject getattr(PyObject obj, CharSequence key)
  {
    return backend.getattr(obj, key);
  }

  public PyObject getattrDefault(PyObject obj, Object key, PyObject defaultValue)
  {
    return backend.getattrDefault(obj, key, defaultValue);
  }

  /**
   * Checks if a Python object has a specific attribute.
   *
   * @param obj the Python object to inspect.
   * @param key the name of the attribute to check.
   * @return {@code true} if the attribute exists, {@code false} otherwise.
   */
  public boolean hasattr(PyObject obj, CharSequence key)
  {
    return backend.hasattrString(obj, key);
  }

  /**
   * Produces a tuple of indices for array-like objects with type safety.
   *
   * @param indices an array of {@link PyIndex} objects representing the
   * indices.
   * @return a new {@link PyTuple} instance containing the indices.
   */
  public PyTuple indices(PySubscript... indices)
  {
    return backend.newTupleFromArray(Arrays.asList(indices));
  }

  /**
   * Checks if an object belongs to one of a set of types.
   *
   * @param obj the object to test.
   * @param types a variable-length array of {@link PyObject} types to check
   * against.
   * @return {@code true} if the object matches any of the types, {@code false}
   * otherwise.
   */
  public boolean isinstance(Object obj, PyObject... types)
  {
    return backend.isinstanceFromArray(obj, types);
  }

  /**
   * Creates a Python iterator from the given object.
   *
   * @param obj the object to convert into an iterator. Must be iterable.
   * @return a new {@link PyIter} instance representing the iterator.
   */
  @SuppressWarnings("unchecked")
  public <T extends PyObject> PyIter<T> iter(Object obj)
  {
    PyIter<? extends PyObject> out = backend.iter(obj);
    return (PyIter<T>) out;
  }

  /**
   * Computes the length of a given Python object by delegating to the Python
   * interpreter backend.
   *
   * <p>
   * This method is a utility that provides access to the Python `len()`
   * function. It calculates the length of the given {@link PyObject} by
   * invoking the appropriate method in the Python interpreter. The behavior of
   * this method depends on the type of the Python object passed as an argument.
   *
   * <p>
   * Examples of supported objects include Python lists, tuples, dictionaries,
   * strings, and other iterable or container types. If the object does not
   * support the `len()` operation, an exception may be thrown.
   *
   * @param obj the Python object whose length is to be computed
   * @return the length of the Python object
   * @throws RuntimeException if the interpreter fails to compute the length or
   * if the object does not support `len()`
   */
  public int len(PyObject obj)
  {
    return backend.len(obj);
  }

  public PyList list()
  {
    return backend.newList();
  }

  /**
   * Invoke Python list on an object.
   *
   * @param object is the object to be converted.
   * @return a new {@link PyObject} representing the Python list.
   */
  public PyList list(Object object)
  {
    return backend.list(object);
  }

  /**
   * Creates a new Python `list` object from the given {@link Iterable}.
   *
   * The elements of the provided iterable will be added to the Python list.
   *
   * @param c the {@link Iterable} whose elements will populate the new Python
   * list.
   * @return a new {@link PyList} instance containing the elements of the
   * iterable.
   */
  public PyList listFromItems(Iterable<? extends Object> c)
  {
    return backend.newListFromIterable(c);
  }

  /**
   * Creates a new Python `list` object from the given a list of objects.
   *
   * @param items is an array whose elements will populate the new Python list.
   * @return a new {@link PyList} instance containing the elements.
   */
  public PyList listFromObjects(Object... items)
  {
    return backend.newListFromArray(items);
  }

  /**
   * Creates a Python memoryview object from the given input.
   *
   * @param obj the object to convert into a memoryview.
   * @return a new {@link PyMemoryView} instance representing the memoryview.
   */
  public PyMemoryView memoryview(Object obj)
  {
    return backend.memoryview(obj);
  }

  /**
   * Retrieves the next item from a Python iterator.
   *
   * @param iter the iterator to retrieve the next item from.
   * @param stop the object to return if the iterator is exhausted.
   * @return the next item as a {@link PyObject}, or the stop object if the
   * iterator is exhausted.
   */
  public PyObject next(PyIter iter, PyObject stop)
  {
    return backend.next(iter, stop);
  }

  public PyObject object()
  {
    return backend.object();
  }

  /**
   * Creates a Python range generator with an endpoint.
   *
   * @param stop the endpoint of the range (exclusive).
   * @return a new {@link PyRange} instance representing the range.
   */
  public PyRange range(int stop)
  {
    return backend.range(stop);
  }

  /**
   * Creates a Python range generator with a start and endpoint.
   *
   * @param start the starting point of the range (inclusive).
   * @param stop the endpoint of the range (exclusive).
   * @return a new {@link PyRange} instance representing the range.
   */
  public PyRange range(int start, int stop)
  {
    return backend.range(start, stop);
  }

  /**
   * Creates a Python range generator with a start, endpoint, and step size.
   *
   * @param start the starting point of the range (inclusive).
   * @param stop the endpoint of the range (exclusive).
   * @param step the step size between elements in the range.
   * @return a new {@link PyRange} instance representing the range.
   */
  public PyRange range(int start, int stop, int step)
  {
    return backend.range(start, stop, step);
  }

  /**
   * Returns the Python string representation of an object.
   *
   * @param obj the object to convert to a string.
   * @return a new {@link PyString} instance representing the string form of the
   * object.
   */
  public PyString repr(Object obj)
  {
    return backend.repr(obj);
  }

  public PySet set()
  {
    return backend.newSet();
  }

  /**
   * Creates a new Python set from the elements of the given {@link Iterable}.
   *
   * @param c an iterable providing elements for the set
   * @param <T> the getType of elements in the iterable
   * @return a new {@code PySet} containing the elements from the iterable
   */
  public <T> PySet set(Iterable<T> c)
  {
    return backend.newSetFromIterable(c);
  }

  /**
   * Sets an attribute on a Python object.
   *
   * @param obj the Python object to modify.
   * @param key the name of the attribute to set.
   * @param value the value to assign to the attribute.
   */
  public void setattr(PyObject obj, CharSequence key, Object value)
  {
    // FIXME we may want special handling for String and Boxed types to 
    // ensure the type that appears is a Python one rather than a 
    // Java one especially on setattr in which the object is to be 
    // held in Python.
    backend.setattrString(obj, key, value);
  }

  /**
   * Creates a single-element slice.
   *
   * This is useful for slicing on a specific element using a tuple.
   *
   * @param start the index of the element to slice on.
   * @return a new {@link PySlice} instance representing the slice.
   */
  public PySlice slice(int start)
  {
    return backend.slice(start, start + 1, null);
  }

  /**
   * Creates a slice with a start and stop index.
   *
   * Passing {@code null} for start or stop indicates no limit. Examples: -
   * `slice(0, 5)` is equivalent to `[0:5]`. - `slice(null, -1)` is equivalent
   * to `[:-1]`. - `slice(3, null)` is equivalent to `[3:]`.
   *
   * @param start the starting index or {@code null}.
   * @param stop the ending index or {@code null}.
   * @return a new {@link PySlice} instance representing the slice.
   */
  public PySlice slice(Integer start, Integer stop)
  {
    return backend.slice(start, stop, null);
  }

  /**
   * Creates a slice with a start, stop, and step size.
   *
   * Passing {@code null} for start, stop, or step indicates no limit. Examples:
   * - `slice(0, 5, 2)` is equivalent to `[0:5:2]`. - `slice(null, -1, 2)` is
   * equivalent to `[:-1:2]`. - `slice(-1, null, -1)` is equivalent to
   * `[-1::-1]`. - `slice(null, null, 2)` is equivalent to `[::2]`.
   *
   * @param start the starting index or {@code null}.
   * @param stop the ending index or {@code null}.
   * @param step the step size or {@code null}.
   * @return a new {@link PySlice} instance representing the slice.
   */
  public PySlice slice(Integer start, Integer stop, Integer step)
  {
    return backend.slice(start, stop, step);
  }

  /**
   * Converts an object to its Python string representation.
   *
   * Equivalent to Python's `str()` function.
   *
   * @param obj the object to convert to a string.
   * @return a new {@link PyString} instance representing the string form of the
   * object.
   */
  public PyString str(Object obj)
  {
    return backend.str(obj);
  }

  /**
   * Creates an empty Python tuple.
   *
   * @param <T> the type of the objects.
   * @return a new {@link PyTuple} instance containing the objects.
   */
  public <T> PyTuple tuple()
  {
    return backend.newTuple();
  }

  /**
   * Creates a Python tuple from a variable-length array of arguments.
   *
   * @param items the objects to include in the tuple.
   * @param <T> the type of the objects.
   * @return a new {@link PyTuple} instance containing the objects.
   */
  public PyTuple tuple(Object... items)
  {
    return backend.newTupleFromArray(Arrays.asList(items));
  }

  /**
   * Creates a new {@code PyTuple} from an {@link Iterable}.
   *
   * @param values an iterator providing the elements to include in the tuple
   * @param <T> the getType of elements in the iterator
   * @return a new {@code PyTuple} containing the elements from the iterator
   */
  public <T> PyTuple tupleFromItems(Iterable<T> values)
  {
    return backend.newTupleFromIterator(values);
  }

  /**
   * Retrieves the Python type of an object.
   *
   * @param obj the object to inspect.
   * @return a {@link PyType} instance representing the type of the object.
   */
  public PyType type(Object obj)
  {
    return backend.type(obj);
  }

  /**
   * Get the `__dict__` attribute of the specified Python object.
   *
   * <p>
   * This method retrieves the `__dict__` attribute, which contains the
   * namespace of the given Python object. The `__dict__` is a mapping object
   * that stores the object's attributes.</p>
   *
   * @param obj the Python object whose `__dict__` attribute is to be retrieved
   * @return a {@link PyDict} representing the `__dict__` attribute of the
   * specified object
   * @throws NullPointerException if the provided object is {@code null}
   */
  public PyDict vars(Object obj)
  {
    return backend.vars(obj);
  }

  public PyDict vars(PyObject obj)
  {
    return backend.vars(obj);
  }

  /**
   * Zips multiple iterable objects into a generator.
   *
   * Equivalent to Python's `zip()` function.
   *
   * @param objects the iterable objects to zip.
   * @return a new {@link PyZip} instance representing the zipped generator.
   */
  public PyZip zip(Iterable<?>... objects)
  {
    return backend.zipFromIterable(Arrays.asList(objects));
  }

}
