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

import org.jpype.bridge.Backend;
import org.jpype.bridge.Interpreter;
import python.protocol.PyCallable;
import python.protocol.PyIndex;
import python.protocol.PyMapping;
import python.protocol.PyIter;

/**
 * Utility class providing built-in functions similar to Python's built-in
 * functions.
 *
 * In general these are set as widely as possible. Many will accept Java objects
 */
public class PyBuiltIn
{

  static Backend instance;

  /**
   * Creates a new Python float object.
   *
   * @param value the double value to be converted to a Python float.
   * @return a new {@link PyFloat} instance representing the given value.
   */
  public static PyFloat $float(double value)
  {
    return backend().newFloat(value);
  }

  /**
   * Creates a new Python integer object.
   *
   * @param value the long value to be converted to a Python integer.
   * @return a new {@link PyInt} instance representing the given value.
   */
  public static PyInt $int(long value)
  {
    return backend().newInt(value);
  }

  /**
   * Utility method to retrieve the active backend instance or throw an
   * exception if the interpreter is not started.
   *
   * <p>
   * This method ensures that the backend instance is properly initialized and
   * active before returning it. If the interpreter has not been started, an
   * {@link IllegalStateException} is thrown to indicate that the backend cannot
   * be accessed.
   * </p>
   *
   * <p>
   * The method also caches the backend instance for future use, avoiding
   * redundant calls to {@link Interpreter#getBackend()}.
   * </p>
   *
   * @return The active {@link Backend} instance.
   * @throws IllegalStateException If the interpreter is not started.
   */
  private static Backend backend()
  {
    if (instance != null)
      return instance;
    if (!Interpreter.getInstance().isStarted())
      throw new IllegalStateException("interpreter is not started");
    instance = Interpreter.getBackend();
    return instance;
  }

  /**
   * Creates a new Python bytes object from the given input.
   *
   * @param obj the object to be converted to a bytes representation.
   * @return a new {@link PyBytes} instance representing the bytes of the
   * object.
   */
  public static PyBytes bytes(Object obj)
  {
    return backend().bytes(obj);
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
  public static PyObject call(PyCallable obj, PyTuple args, PyDict kwargs)
  {
    return backend().call(obj, args, kwargs);
  }

  /**
   * Deletes an attribute from a Python object.
   *
   * @param obj the Python object from which the attribute will be removed.
   * @param key the name of the attribute to delete.
   */
  public static void delattr(PyObject obj, CharSequence key)
  {
    backend().delattrString(obj, key);
  }

  public static PyDict dict()
  {
    return backend().newDict();
  }

  /**
   * Returns a list of attribute names for a Python object.
   *
   * @param obj the Python object to inspect.
   * @return a {@link PyList} containing the attribute names.
   */
  public static PyList dir(PyObject obj)
  {
    return backend().dir(obj);
  }

  /**
   * Creates a Python enumerate object from the given iterable.
   *
   * @param obj the iterable to enumerate.
   * @return a new {@link PyEnumerate} instance.
   */
  public static PyEnumerate enumerate(PyObject obj)
  {
    return backend().enumerate(obj);
  }

  /**
   * Creates a Python enumerate object from the given Java iterable.
   *
   * @param obj the Java iterable to enumerate.
   * @return a new {@link PyEnumerate} instance.
   */
  public static PyEnumerate enumerate(Iterable obj)
  {
    return backend().enumerate(obj);
  }

  /**
   * Evaluates a Python expression in the given global and local namespaces.
   *
   * @param statement the Python expression to evaluate.
   * @param globals the global namespace as a {@link PyDict}.
   * @param locals the local namespace as a {@link PyMapping}.
   * @return the result of the evaluation as a {@link PyObject}.
   */
  public static PyObject eval(CharSequence statement, PyDict globals, PyMapping locals)
  {
    return backend().eval(statement, globals, locals);
  }

  /**
   * Executes a Python statement in the given global and local namespaces.
   *
   * @param statement the Python statement to execute.
   * @param globals the global namespace as a {@link PyDict}.
   * @param locals the local namespace as a {@link PyMapping}.
   */
  public static void exec(CharSequence statement, PyDict globals, PyMapping locals)
  {
    backend().eval(statement, globals, locals);
  }

  /**
   * Retrieves the value of an attribute from a Python object.
   *
   * @param obj the Python object to inspect.
   * @param key the name of the attribute to retrieve.
   * @return the value of the attribute as a {@link PyObject}.
   */
  public static PyObject getattr(PyObject obj, CharSequence key)
  {
    return backend().getattrString(obj, key);
  }

  /**
   * Retrieves the value of an attribute from a Python object.
   *
   * @param obj the Python object to inspect.
   * @param key the name of the attribute to retrieve.
   * @return the value of the attribute as a {@link PyObject}.
   */
  public static PyObject getattr(PyObject obj, Object key)
  {
    return backend().getattrObject(obj, key);
  }

  public static PyObject getattrDefault(PyObject obj, Object key, PyObject defaultValue)
  {
    return backend().getattrDefault(obj, key);
  }

  /**
   * Checks if a Python object has a specific attribute.
   *
   * @param obj the Python object to inspect.
   * @param key the name of the attribute to check.
   * @return {@code true} if the attribute exists, {@code false} otherwise.
   */
  public static boolean hasattr(PyObject obj, CharSequence key)
  {
    return backend().hasattrString(obj, key);
  }

  /**
   * Produces a tuple of indices for array-like objects with type safety.
   *
   * @param indices an array of {@link PyIndex} objects representing the
   * indices.
   * @return a new {@link PyTuple} instance containing the indices.
   */
  public static PyTuple indices(PyIndex[] indices)
  {
    return backend().newTupleFromArray(indices);
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
  public static boolean isinstance(Object obj, PyObject... types)
  {
    return backend().isinstanceFromArray(obj, types);
  }

  /**
   * Creates a Python iterator from the given object.
   *
   * @param obj the object to convert into an iterator. Must be iterable.
   * @return a new {@link PyIter} instance representing the iterator.
   */
  public static PyIter iter(Object obj)
  {
    return backend().iter(obj);
  }

  /**
   * Computes the length of a given Python object by delegating to the Python
   * interpreter backend.
   *
   * <p>
   * This method is a static utility that provides access to the Python `len()`
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
  public static int len(PyObject obj)
  {
    return backend().len(obj);
  }

  public static PyList list()
  {
    return backend().newList();
  }

  /**
   * Invoke Python list on an object.
   *
   * @param object is the object to be converted.
   * @return a new {@link PyObject} representing the Python list.
   */
  public static PyList list(Object object)
  {
    return backend().list(object);
  }

  /**
   * Creates a Python memoryview object from the given input.
   *
   * @param obj the object to convert into a memoryview.
   * @return a new {@link PyMemoryView} instance representing the memoryview.
   */
  public static PyMemoryView memoryview(Object obj)
  {
    return backend().memoryview(obj);
  }

  /**
   * Retrieves the next item from a Python iterator.
   *
   * @param iter the iterator to retrieve the next item from.
   * @param stop the object to return if the iterator is exhausted.
   * @return the next item as a {@link PyObject}, or the stop object if the
   * iterator is exhausted.
   */
  public static PyObject next(PyIter iter, PyObject stop)
  {
    return backend().next(iter, stop);
  }

  /**
   * Creates a Python range generator with an endpoint.
   *
   * @param stop the endpoint of the range (exclusive).
   * @return a new {@link PyRange} instance representing the range.
   */
  public static PyRange range(int stop)
  {
    return backend().range(stop);
  }

  /**
   * Creates a Python range generator with a start and endpoint.
   *
   * @param start the starting point of the range (inclusive).
   * @param stop the endpoint of the range (exclusive).
   * @return a new {@link PyRange} instance representing the range.
   */
  public static PyRange range(int start, int stop)
  {
    return backend().range(start, stop);
  }

  /**
   * Creates a Python range generator with a start, endpoint, and step size.
   *
   * @param start the starting point of the range (inclusive).
   * @param stop the endpoint of the range (exclusive).
   * @param step the step size between elements in the range.
   * @return a new {@link PyRange} instance representing the range.
   */
  public static PyRange range(int start, int stop, int step)
  {
    return backend().range(start, stop, step);
  }

  /**
   * Returns the Python string representation of an object.
   *
   * @param obj the object to convert to a string.
   * @return a new {@link PyString} instance representing the string form of the
   * object.
   */
  public static PyString repr(Object obj)
  {
    return backend().repr(obj);
  }

  public static PySet set()
  {
    return backend().newSet();
  }

  /**
   * Sets an attribute on a Python object.
   *
   * @param obj the Python object to modify.
   * @param key the name of the attribute to set.
   * @param value the value to assign to the attribute.
   */
  public static void setattr(PyObject obj, CharSequence key, Object value)
  {
    // FIXME we may want special handling for String and Boxed types to 
    // ensure the type that appears is a Python one rather than a 
    // Java one especially on setattr in which the object is to be 
    // held in Python.
    backend().setattrString(obj, key, value);
  }

  /**
   * Creates a single-element slice.
   *
   * This is useful for slicing on a specific element using a tuple.
   *
   * @param start the index of the element to slice on.
   * @return a new {@link PySlice} instance representing the slice.
   */
  public static PySlice slice(int start)
  {
    return backend().slice(start, start + 1, null);
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
  public static PySlice slice(Integer start, Integer stop)
  {
    return backend().slice(start, stop, null);
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
  public static PySlice slice(Integer start, Integer stop, Integer step)
  {
    return backend().slice(start, stop, step);
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
  public static PyString str(Object obj)
  {
    return backend().str(obj);
  }

  /**
   * Creates an empty Python tuple.
   *
   * @param args the objects to include in the tuple.
   * @param <T> the type of the objects.
   * @return a new {@link PyTuple} instance containing the objects.
   */
  public static <T> PyTuple tuple()
  {
    return backend().newTuple();
  }

  /**
   * Creates a Python tuple from a variable-length array of arguments.
   *
   * @param args the objects to include in the tuple.
   * @param <T> the type of the objects.
   * @return a new {@link PyTuple} instance containing the objects.
   */
  public static <T> PyTuple tuple(T... args)
  {
    return backend().newTupleFromArray(args);
  }

  /**
   * Retrieves the Python type of an object.
   *
   * @param obj the object to inspect.
   * @return a {@link PyType} instance representing the type of the object.
   */
  public static PyType type(Object obj)
  {
    return backend().type(obj);
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
  public static PyDict vars(Object obj)
  {
    return backend().vars(obj);
  }

  /**
   * Zips multiple iterable objects into a generator.
   *
   * Equivalent to Python's `zip()` function.
   *
   * @param objects the iterable objects to zip.
   * @return a new {@link PyZip} instance representing the zipped generator.
   */
  public static PyZip zip(Iterable... objects)
  {
    return backend().zipFromIterable(objects);
  }
}
