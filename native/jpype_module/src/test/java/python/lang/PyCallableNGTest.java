// --- file: python/lang/PyCallableNGTest.java ---
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

import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises {@link PyCallable}'s own synchronous default methods (call,
 * callWithKwargs, getDocString, getSignature, isCallable). Unlike
 * {@code PyCallableAsyncNGTest} (which only covers the async/timeout
 * variants), this targets the plain synchronous surface.
 *
 * <p>
 * Both {@link PyFunction} and {@link PyLambda} - the two interfaces that
 * back ordinary Python {@code def}/{@code lambda} objects - are pure marker
 * interfaces that add nothing and override nothing from {@link PyCallable},
 * so a plain Python function object cast to {@code PyCallable} reaches
 * these default method bodies directly.
 */
public class PyCallableNGTest extends PyTestHarness
{

  @Test
  public void testCallWithTuple()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y: x + y");

    PyObject result = fn.call(context.tuple(2, 3));

    assertEquals(result.toString(), "5");
  }

  @Test
  public void testCallWithTupleAndKwargs()
  {
    context.exec("def _pi_callable_add(x, y=10): return x + y\n");
    PyCallable fn = (PyCallable) context.eval("_pi_callable_add");

    PyDict kwargs = context.dict();
    kwargs.putAny(context.str("y"), context.$int(5));
    PyObject result = fn.call(context.tuple(1), kwargs);

    assertEquals(result.toString(), "6");
  }

  @Test
  public void testCallWithVarargs()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y, z: x * y * z");

    PyObject result = fn.call(2, 3, 4);

    assertEquals(result.toString(), "24");
  }

  @Test
  public void testCallWithKwargsOnly()
  {
    context.exec("def _pi_callable_greet(name='world'): return 'hello ' + name\n");
    PyCallable fn = (PyCallable) context.eval("_pi_callable_greet");

    PyDict kwargs = context.dict();
    kwargs.putAny(context.str("name"), context.str("jpype"));
    PyObject result = fn.callWithKwargs(kwargs);

    assertEquals(result.toString(), "hello jpype");
  }

  @Test
  public void testGetDocString()
  {
    context.exec(
            "def _pi_callable_documented():\n"
            + "    \"\"\"a friendly docstring\"\"\"\n"
            + "    return None\n");
    PyCallable fn = (PyCallable) context.eval("_pi_callable_documented");

    String doc = fn.getDocString();

    assertNotNull(doc);
    assertTrue(doc.contains("a friendly docstring"));
  }

  @Test
  public void testGetSignature()
  {
    context.exec("def _pi_callable_sig(a, b, c=1): return None\n");
    PyCallable fn = (PyCallable) context.eval("_pi_callable_sig");

    PyObject signature = fn.getSignature();

    assertNotNull(signature);
    assertTrue(signature.toString().contains("a"));
    assertTrue(signature.toString().contains("b"));
    assertTrue(signature.toString().contains("c"));
  }

  @Test
  public void testIsCallableTrueForFunction()
  {
    PyCallable fn = (PyCallable) context.eval("lambda: 1");

    assertTrue(fn.isCallable());
  }

  @Test
  public void testCallBuilderExecute()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y: x - y");

    PyObject result = fn.invoker().arg(10).arg(3).execute();

    assertEquals(result.toString(), "7");
  }
}
