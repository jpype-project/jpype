// --- file: org/jpype/script/JPypeScriptEngine.java ---
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
package org.jpype.script;

import java.io.IOException;
import java.io.Reader;
import java.lang.reflect.Proxy;
import java.util.Map;
import javax.script.AbstractScriptEngine;
import javax.script.Bindings;
import javax.script.Invocable;
import javax.script.ScriptContext;
import javax.script.ScriptEngineFactory;
import javax.script.ScriptException;
import javax.script.SimpleBindings;
import org.jpype.MainInterpreter;
import org.jpype.Script;
import python.exceptions.PyBaseException;
import python.exceptions.PySyntaxError;
import python.lang.PyCallable;
import python.lang.PyDict;
import python.lang.PyFloat;
import python.lang.PyInt;
import python.lang.PyJavaObject;
import python.lang.PyObject;
import python.lang.PyString;

/**
 * JSR-223 {@link javax.script.ScriptEngine} adapter over JPype's embedded
 * Python interpreter - a trivial compatibility shim so tools that discover
 * engines via {@code ScriptEngineManager}/{@code META-INF/services} (rather
 * than JPype's own API) can drive the same interpreter.
 *
 * <p>
 * The engine keeps one persistent {@link Script} - a single Python
 * {@code globals}/{@code locals} namespace, the same building block
 * {@code PyTestHarness} uses - alive for the life of the engine, rather than
 * building a fresh namespace per {@code eval} call: a {@code def}'d function
 * closes over its defining {@code globals} dict, so that dict (and anything
 * reachable from it) has to keep living for as long as the function might
 * still be called, which a call-scoped dict can't guarantee. Each
 * {@link #eval(String, ScriptContext)} call pushes the current
 * {@code GLOBAL_SCOPE}/{@code ENGINE_SCOPE} {@link Bindings} into that
 * namespace (lower precedence first, so {@code ENGINE_SCOPE} shadows
 * {@code GLOBAL_SCOPE} the way the JSR-223 spec requires), runs the script,
 * then copies every resulting namespace entry back into {@code ENGINE_SCOPE}
 * so state the script defined or reassigned - including new functions - is
 * visible to the next call and to {@link #invokeFunction}.
 *
 * <p>
 * Unlike jpype's core {@code PyObject}-typed APIs - which stay in
 * {@code PyObject} throughout so Python's own autoboxing semantics are
 * preserved for arbitrary two-way traffic - this class marshals scalar
 * results ({@code str}/{@code int}/{@code float}, plus {@code None} which
 * already crosses the bridge as Java {@code null}) to native Java
 * {@code String}/{@code Long}/{@code Double}, and unboxes a
 * {@code PyJavaObject} (a Java object previously handed into Python) back to
 * the original Java object, at every Java-facing return
 * point ({@link #eval}, {@link #invokeFunction}, {@link #invokeMethod}, and
 * the {@code ENGINE_SCOPE} {@link Bindings} written back after each
 * {@code eval}), because {@code javax.script.ScriptEngine} is a Java
 * standard interface with its own marshalling contract, not another
 * jpype-specific bridge API. Non-scalar results (a {@code dict},
 * {@code list}, or other Python object) have no single natural Java type to
 * fall into, so they're still returned as {@code PyObject}.
 */
public class JPypeScriptEngine extends AbstractScriptEngine implements Invocable
{

  private final JPypeScriptEngineFactory factory;
  private final Script scope;

  JPypeScriptEngine(JPypeScriptEngineFactory factory)
  {
    this.factory = factory;
    MainInterpreter interpreter = MainInterpreter.getInstance();
    if (!interpreter.isStarted())
      interpreter.start();
    this.scope = new Script(interpreter);
  }

  @Override
  public ScriptEngineFactory getFactory()
  {
    return factory;
  }

  @Override
  public Bindings createBindings()
  {
    return new SimpleBindings();
  }

  @Override
  public Object eval(Reader reader, ScriptContext context) throws ScriptException
  {
    return eval(readAll(reader), context);
  }

  /**
   * Runs {@code script} as a Python expression first (so a plain expression
   * like {@code "1 + 1"} returns its value, matching
   * {@code ScriptEngine.eval}'s contract) and falls back to statement
   * execution - returning {@code null}, since Python's {@code exec} has no
   * result - whenever the source isn't expression-shaped.
   */
  @Override
  public Object eval(String script, ScriptContext context) throws ScriptException
  {
    Bindings engineScope = context.getBindings(ScriptContext.ENGINE_SCOPE);
    Bindings globalScope = context.getBindings(ScriptContext.GLOBAL_SCOPE);

    PyDict globals = scope.globals();
    pushBindings(globals, globalScope);
    pushBindings(globals, engineScope);

    try
    {
      Object result;
      try
      {
        result = toNative(scope.eval(script));
      } catch (PySyntaxError notAnExpression)
      {
        scope.exec(script);
        result = null;
      }
      writeBack(globals, engineScope);
      return result;
    } catch (PyBaseException ex)
    {
      throw new ScriptException(ex);
    }
  }

  /**
   * Marshals a Python scalar to its native Java equivalent - see the class
   * doc for why this engine does that at its Java-facing edges when the rest
   * of jpype deliberately doesn't. Anything without a single natural Java
   * type (a {@code dict}, {@code list}, function, or other Python object) is
   * returned unchanged.
   */
  private static Object toNative(PyObject value)
  {
    if (value instanceof PyFloat)
      return ((PyFloat) value).toNumber();
    if (value instanceof PyInt)
      return ((PyInt) value).toNumber();
    if (value instanceof PyString)
      return value.toString();
    if (value instanceof PyJavaObject)
      return ((PyJavaObject) value).get();
    return value;
  }

  private static void pushBindings(PyDict globals, Bindings bindings)
  {
    if (bindings == null)
      return;
    for (Map.Entry<String, Object> entry : bindings.entrySet())
      globals.putAny(entry.getKey(), entry.getValue());
  }

  @Override
  public Object invokeFunction(String name, Object... args) throws ScriptException, NoSuchMethodException
  {
    return invoke(lookup(name), name, args);
  }

  @Override
  public Object invokeMethod(Object thiz, String name, Object... args) throws ScriptException, NoSuchMethodException
  {
    if (!(thiz instanceof PyObject))
      throw new IllegalArgumentException("'thiz' must be an instance of " + PyObject.class.getName());
    PyObject obj = (PyObject) thiz;
    if (!obj.getAttributes().contains(name))
      throw new NoSuchMethodException(name);
    return invoke(obj.getAttributes().get(name), name, args);
  }

  @Override
  public <T> T getInterface(Class<T> clasz)
  {
    return proxy(clasz, (name, args) -> invokeFunction(name, args));
  }

  @Override
  public <T> T getInterface(Object thiz, Class<T> clasz)
  {
    if (!(thiz instanceof PyObject))
      throw new IllegalArgumentException("'thiz' must be an instance of " + PyObject.class.getName());
    return proxy(clasz, (name, args) -> invokeMethod(thiz, name, args));
  }

  private interface Invoker
  {
    Object invoke(String name, Object[] args) throws ScriptException, NoSuchMethodException;
  }

  private <T> T proxy(Class<T> clasz, Invoker invoker)
  {
    if (clasz == null || !clasz.isInterface())
      throw new IllegalArgumentException("'clasz' must be an interface");
    return clasz.cast(Proxy.newProxyInstance(clasz.getClassLoader(), new Class<?>[]
    {
      clasz
    }, (proxy, method, args) ->
    {
      try
      {
        return invoker.invoke(method.getName(), args == null ? new Object[0] : args);
      } catch (ScriptException | NoSuchMethodException ex)
      {
        throw new RuntimeException(ex);
      }
    }));
  }

  private Object invoke(PyObject fn, String name, Object[] args) throws ScriptException, NoSuchMethodException
  {
    if (!(fn instanceof PyCallable))
      throw new NoSuchMethodException(name);
    try
    {
      // PyCallable.call(Object...)/.call(PyTuple) pass a Java `null` as
      // kwargs, which crashes native call dispatch (PyValueError: "method
      // called on null object") for a plain function/lambda obtained via
      // eval() - unlike PyBuiltIn.call(PyCallable, PyTuple, PyDict), which
      // takes an explicit (possibly empty) kwargs dict and works. Route
      // through the builtin form to avoid the null-kwargs path.
      return toNative(scope.call((PyCallable) fn, scope.tuple(args), scope.dict()));
    } catch (PyBaseException ex)
    {
      throw new ScriptException(ex);
    }
  }

  private PyObject lookup(String name) throws NoSuchMethodException
  {
    Bindings engineScope = getContext().getBindings(ScriptContext.ENGINE_SCOPE);
    Object value = engineScope == null ? null : engineScope.get(name);
    if (!(value instanceof PyObject))
      throw new NoSuchMethodException(name);
    return (PyObject) value;
  }

  /**
   * Copies every entry of a post-execution Python namespace back into the
   * {@link Bindings} it was built from, so names the script defined or
   * reassigned become visible via {@code Bindings.get()}. Skips
   * {@code __builtins__}, which {@code exec}/{@code eval} inject into any
   * globals dict that lacks it - an implementation artifact, not something
   * the caller put there.
   */
  private static void writeBack(PyDict namespace, Bindings target)
  {
    if (target == null)
      return;
    for (Map.Entry<PyObject, PyObject> entry : namespace.entrySet())
    {
      String key = entry.getKey().toString();
      if ("__builtins__".equals(key))
        continue;
      target.put(key, toNative(entry.getValue()));
    }
  }

  private static String readAll(Reader reader) throws ScriptException
  {
    try
    {
      StringBuilder sb = new StringBuilder();
      char[] buf = new char[4096];
      int n;
      while ((n = reader.read(buf)) != -1)
        sb.append(buf, 0, n);
      return sb.toString();
    } catch (IOException ex)
    {
      throw new ScriptException(ex);
    }
  }
}
