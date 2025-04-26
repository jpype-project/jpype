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
package python.protocol;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map;
import java.util.concurrent.Future;
import org.jpype.bridge.Interpreter;
import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyTuple;

/**
 * Protocol for Python objects that act as callable entities.
 *
 * This interface defines methods for invoking Python objects as functions,
 * handling positional and keyword arguments, and supporting asynchronous calls.
 * It also provides utility methods for retrieving metadata about the callable,
 * such as its documentation string and signature.
 *
 * To allow for method overloading, the entry point for calls must remain
 * private.
 */
public interface PyCallable extends PyProtocol
{

  /**
   * Creates a {@link CallBuilder} for constructing and executing calls to this
   * callable object.
   *
   * The {@link CallBuilder} allows for flexible configuration of arguments and
   * keyword arguments before executing the call.
   *
   * @return a new {@link CallBuilder} instance associated with this callable
   */
  default CallBuilder call()
  {
    return new CallBuilder(this);
  }

  /**
   * Invokes the callable Python object with the specified positional and
   * keyword arguments.
   *
   * @param args the positional arguments as a {@link PyTuple}
   * @param kwargs the keyword arguments as a {@link PyDict}
   * @return the result of the call as a {@link PyObject}
   */
  default PyObject call(PyTuple args, PyDict kwargs)
  {
    return Interpreter.getBackend().call(this, args, kwargs);
  }

  /**
   * Invokes the callable Python object with the specified positional arguments.
   *
   * @param args the positional arguments as a {@link PyTuple}
   * @return the result of the call as a {@link PyObject}
   */
  default PyObject call(PyTuple args)
  {
    return Interpreter.getBackend().call(this, args, null);
  }

  /**
   * Invokes the callable Python object with the specified arguments as a Java
   * array.
   *
   * @param args the positional arguments as a Java array
   * @return the result of the call as a {@link PyObject}
   */
  default PyObject call(Object... args)
  {
    return Interpreter.getBackend().call(this, PyTuple.of(args), null);
  }

  /**
   * Invokes the callable Python object asynchronously with the specified
   * arguments.
   *
   * @param args the positional arguments as a {@link PyTuple}
   * @param kwargs the keyword arguments as a {@link PyDict}
   * @return a {@link Future} representing the result of the asynchronous call
   */
  default Future<PyObject> callAsync(PyTuple args, PyDict kwargs)
  {
    return Interpreter.getBackend().callAsync(this, args, kwargs);
  }

  /**
   * Invokes the callable Python object asynchronously with a timeout.
   *
   * @param args the positional arguments as a {@link PyTuple}
   * @param kwargs the keyword arguments as a {@link PyDict}
   * @param timeout the maximum time (in milliseconds) to wait for the call to
   * complete
   * @return a {@link Future} representing the result of the asynchronous call
   */
  default Future<PyObject> callAsyncWithTimeout(PyTuple args, PyDict kwargs, long timeout)
  {
    return Interpreter.getBackend().callAsyncWithTimeout(this, args, kwargs, timeout);
  }

  /**
   * Invokes the callable Python object with keyword-only arguments.
   *
   * @param kwargs the keyword arguments as a {@link PyDict}
   * @return the result of the call as a {@link PyObject}
   */
  default PyObject callWithKwargs(PyDict kwargs)
  {
    return Interpreter.getBackend().call(this, PyTuple.empty(), kwargs);
  }

  /**
   * Retrieves the documentation string (docstring) of the callable Python
   * object.
   *
   * @return the docstring as a {@link String}, or {@code null} if no
   * documentation is available
   */
  default String getDocString()
  {
    return Interpreter.getBackend().getDocString(this);
  }

  /**
   * Retrieves the signature of the callable Python object.
   *
   * @return the signature as a {@link PyObject}
   */
  default PyObject getSignature()
  {
    return Interpreter.getBackend().getSignature(this);
  }

  /**
   * Checks whether this Python object is callable.
   *
   * @return {@code true} if the object is callable, {@code false} otherwise
   */
  default boolean isCallable()
  {
    return Interpreter.getBackend().isCallable(this);
  }

  /**
   * Retrieves the type or category of the callable Python object.
   *
   * @return a {@link String} representing the callable's type
   */
  default String getCallableType()
  {
    return Interpreter.getBackend().getCallableType(this);
  }

  // Nested CallBuilder class documentation
  /**
   * A builder for constructing and executing calls to a {@link PyCallable}.
   *
   * The {@link CallBuilder} allows for adding positional and keyword arguments
   * incrementally and provides methods for executing the call synchronously or
   * asynchronously.
   */
  public static class CallBuilder
  {

    final PyCallable callable;
    final ArrayList<Object> jargs = new ArrayList<>();
    final ArrayList<Map.Entry<Object, ? extends PyObject>> jkwargs = new ArrayList<>();

    /**
     * Creates a new {@link CallBuilder} for the specified {@link PyCallable}.
     *
     * @param callable the callable object to associate with this builder
     */
    public CallBuilder(PyCallable callable)
    {
      this.callable = callable;
    }

    /**
     * Adds a single positional argument to the call sequence.
     *
     * @param arg the argument to add
     * @return this {@link CallBuilder} instance for chaining
     */
    public CallBuilder arg(Object arg)
    {
      jargs.add(arg);
      return this;
    }

    /**
     * Adds multiple positional arguments to the call sequence.
     *
     * @param args the arguments to add
     * @return this {@link CallBuilder} instance for chaining
     */
    public CallBuilder arg(Object... args)
    {
      jargs.addAll(Arrays.asList(args));
      return this;
    }

    /**
     * Adds a single keyword argument to the call sequence.
     *
     * @param name the name of the keyword argument
     * @param value the value of the keyword argument
     * @return this {@link CallBuilder} instance for chaining
     */
    public CallBuilder kwarg(CharSequence name, Object value)
    {
      jkwargs.add(new CallBuilderEntry(name, value));
      return this;
    }

    /**
     * Adds multiple keyword arguments to the call sequence.
     *
     * @param kwargs a {@link Map} containing keyword arguments
     * @return this {@link CallBuilder} instance for chaining
     */
    public CallBuilder kwargs(Map<Object, PyObject> kwargs)
    {
      for (Map.Entry<Object, PyObject> entry : kwargs.entrySet())
      {
        this.kwarg(entry.getKey().toString(), entry.getValue());
      }
      return this;
    }

    /**
     * Clears all arguments and keyword arguments from the call sequence.
     *
     * @return this {@link CallBuilder} instance for chaining
     */
    public CallBuilder clear()
    {
      jargs.clear();
      jkwargs.clear();
      return this;
    }

    /**
     * Executes the call synchronously with the current arguments and keyword
     * arguments.
     *
     * @return the result of the call as a {@link PyObject}
     */
    public PyObject execute()
    {
      return callable.call(PyTuple.of(jargs), PyDict.of(jkwargs));
    }

    /**
     * Executes the call asynchronously with the current arguments and keyword
     * arguments.
     *
     * @return a {@link Future} representing the result of the asynchronous call
     */
    public Future<PyObject> executeAsync()
    {
      return callable.callAsync(PyTuple.of(jargs), PyDict.of(jkwargs));
    }

    /**
     * Executes the call asynchronously with a timeout.
     *
     * @param timeout the maximum time (in milliseconds) to wait for the call to
     * complete
     * @return a {@link Future} representing the result of the asynchronous call
     */
    public Future<PyObject> executeAsync(long timeout)
    {
      return callable.callAsyncWithTimeout(PyTuple.of(jargs), PyDict.of(jkwargs), timeout);
    }
  }

  /**
   * Represents a single entry in the keyword arguments for a call.
   */
  public static class CallBuilderEntry implements Map.Entry
  {

    private final Object key;
    private final Object value;

    /**
     * Creates a new immutable entry for a keyword argument.
     *
     * @param key the key of the keyword argument
     * @param value the value of the keyword argument
     */
    public CallBuilderEntry(Object key, Object value)
    {
      this.key = key;
      this.value = value;
    }

    @Override
    public Object getKey()
    {
      return key;
    }

    @Override
    public Object getValue()
    {
      return value;
    }

    @Override
    public PyObject setValue(Object value)
    {
      throw new UnsupportedOperationException("Entry is immutable");
    }
  }
}
