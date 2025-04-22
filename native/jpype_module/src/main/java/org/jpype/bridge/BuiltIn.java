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

import python.lang.PyBytes;
import python.lang.PyDict;
import python.lang.PyEnumerate;
import python.lang.PyList;
import python.lang.PyMemoryView;
import python.lang.PyObject;
import python.lang.PyRange;
import python.lang.PySlice;
import python.lang.PyString;
import python.lang.PyTuple;
import python.lang.PyType;
import python.lang.PyZip;
import python.protocol.PyCallable;
import python.protocol.PyMapping;
import python.protocol.PyIter;

/**
 * Collection of static methods available in Python.
 *
 * In general these are set as widely as possible. Many will accept Java objects
 */
public class BuiltIn
{

  /**
   * Create a bytes representation of an object.
   *
   * @param obj
   * @return a new bytes object.
   */
  public static PyBytes bytes(Object obj)
  {
    return Interpreter.backend.bytes(obj);
  }

  public static PyObject call(PyCallable obj, PyTuple args, PyDict kwargs)
  {
    return Interpreter.backend.call(obj, args, kwargs);
  }

  public static PyList dir(PyObject obj)
  {
    return Interpreter.backend.dir(obj);
  }

  public static void delattr(PyObject obj, CharSequence key)
  {
    Interpreter.backend.delattr(obj, key);
  }

  public static PyObject next(PyIter iter, PyObject stop)
  {
    return Interpreter.backend.next(iter, stop);
  }

  public static PyEnumerate enumerate(PyObject obj)
  {
    return Interpreter.backend.enumerate(obj);
  }

  public static PyEnumerate enumerate(Iterable obj)
  {
    return Interpreter.backend.enumerate(obj);
  }

  public static PyObject eval(CharSequence statement, PyDict globals, PyMapping locals)
  {
    return Interpreter.backend.eval(statement, globals, locals);
  }

  public static void exec(CharSequence statement, PyDict globals, PyMapping locals)
  {
    Interpreter.backend.eval(statement, globals, locals);
  }

  public static PyObject getattr(PyObject obj, CharSequence key)
  {
    return Interpreter.backend.getattr(obj, key);
  }

  public static boolean hasattr(PyObject obj, CharSequence key)
  {
    return Interpreter.backend.hasattr(obj, key);
  }

  /**
   * Check if an object belongs to one of a set of types.
   *
   * @param obj is the object to test.
   * @param types are a set of PyType objects.
   * @return true if obj is a member.
   */
  public static boolean isinstance(Object obj, PyObject... types)
  {
    return Interpreter.backend.isinstance(obj, types);
  }

  public static PyIter iter(Object obj)
  {
    return Interpreter.backend.iter(obj);
  }

  public static PyList list(Iterable<Object> objects)
  {
    return Interpreter.backend.list(objects);
  }

  public static PyObject list(Object... objects)
  {
    return Interpreter.backend.list(objects);
  }

  /**
   * Create a new memoryview of an object.
   *
   * @param obj is the object to convert.
   * @return a new memoryview.
   */
  public static PyMemoryView memoryview(Object obj)
  {
    return Interpreter.backend.memoryview(obj);
  }

  /**
   * Produce a generator with a defined end point.
   *
   * @param stop is the end point for the range.
   * @return a new generator.
   */
  public static PyRange range(int stop)
  {
    return Interpreter.backend.range(stop);
  }

  /**
   * Produce a generator covering a range.
   *
   * @param start is the start point for the range.
   * @param stop is the end point for the range.
   * @return a new generator.
   */
  public static PyRange range(int start, int stop)
  {
    return Interpreter.backend.range(start, stop);
  }

  /**
   * Produce a generator covering a range with a step.
   *
   * @param start is the start point for the range.
   * @param stop is the end point for the range.
   * @param step is the distance to skip.
   * @return a new generator.
   */
  public static PyRange range(int start, int stop, int step)
  {
    return Interpreter.backend.range(start, stop, step);
  }

  public static PyString repr(Object obj)
  {
    return Interpreter.backend.repr(obj);
  }

  public static void setattr(PyObject obj, CharSequence key, Object value)
  {
    Interpreter.backend.setattr(obj, key, value);
  }

  /**
   * Create a single element slice.
   *
   * This is useful using a tuple to slice on.
   *
   * @param start is the element to slice on.
   * @return a new slice.
   */
  public static PySlice slice(int start)
  {
    return Interpreter.backend.slice(start, start + 1, null);
  }

  /**
   * Create a slice.
   *
   * Passing nulls to slice indicates no limit.
   * <ul>
   * <li>slice(0,5) is [0:5].</li>
   * <li>slice(null, -1) is [:-1]</li>
   * <li>slice(3,null) is [3:]</li>
   * </ul>
   *
   * @param start is the lower limit or null.
   * @param stop is the upper limit or null.
   * @return a new slice object.
   */
  public static PySlice slice(Integer start, Integer stop)
  {
    return Interpreter.backend.slice(start, stop, null);
  }

  /**
   * Create a slice with a step.
   *
   * Passing nulls to slice indicates no limit.
   * <ul>
   * <li>slice(0,5,2) is [0:5:2].</li>
   * <li>slice(null, -1,2) is [:-1:2]</li>
   * <li>slice(-1,null,-1) is [-1::-1]</li>
   * <li>slice(null,null,2) is [::2]</li>
   * </ul>
   *
   * @param start is the lower limit or null.
   * @param stop is the upper limit or null.
   * @param step
   * @return a new slice object.
   */
  public static PySlice slice(Integer start, Integer stop, Integer step)
  {
    return Interpreter.backend.slice(start, stop, step);
  }

  public static PyString str(Object obj)
  {
    return Interpreter.backend.str(obj);
  }

  public static <T> PyTuple tuple(T... args)
  {
    return Interpreter.backend.tuple(args);
  }

  public static <T> PyTuple tuple(Iterable<T>... args)
  {
    return Interpreter.backend.tuple(args);
  }

  public static PyType type(Object obj)
  {
    return Interpreter.backend.type(obj);
  }

  /**
   * Zip a set of items into a generator.
   *
   * @param objects are the object to zip which must be iterable.
   * @return a new generator
   */
  public static PyZip zip(PyObject... objects)
  {
    return Interpreter.backend.zip(objects);
  }

}
