// --- file: python/lang/PyCallBuilderNGTest.java ---
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

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyCallable.CallBuilder;
import python.lang.PyCallable.CallBuilderEntry;

public class PyCallBuilderNGTest extends PyTestHarness
{

  @Test
  public void testArgOnly()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y: x + y");
    PyObject result = fn.invoker().arg(2).arg(3).execute();
    assertEquals(result.toString(), "5");
  }

  @Test
  public void testArgsVarargs()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y, z: x + y + z");
    PyObject result = fn.invoker().args(1, 2, 3).execute();
    assertEquals(result.toString(), "6");
  }

  @Test
  public void testKwargOnly()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y=10: x - y");
    PyObject result = fn.invoker().arg(3).kwarg("y", 1).execute();
    assertEquals(result.toString(), "2");
  }

  @Test
  public void testKwargsMap()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y: x * y");
    Map<Object, PyObject> kwargs = new LinkedHashMap<>();
    kwargs.put("x", context.eval("3"));
    kwargs.put("y", context.eval("4"));
    PyObject result = fn.invoker().kwargs(kwargs).execute();
    assertEquals(result.toString(), "12");
  }

  @Test
  public void testClearResetsBuilder()
  {
    PyCallable fn = (PyCallable) context.eval("lambda *a, **k: (len(a), len(k))");
    CallBuilder builder = fn.invoker().arg(1).arg(2).kwarg("z", 3);
    builder.clear();
    PyObject result = builder.execute();
    assertEquals(result.toString(), "(0, 0)");
  }

  @Test
  public void testChainingReturnsSameBuilder()
  {
    PyCallable fn = (PyCallable) context.eval("lambda x: x");
    CallBuilder builder = fn.invoker();
    assertSame(builder.arg(1), builder);
    assertSame(builder.args(2, 3), builder);
    assertSame(builder.kwarg("k", 1), builder);
    assertSame(builder.clear(), builder);
  }

  @Test(timeOut = 15000)
  public void testExecuteAsync() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y: x + y");
    Future<PyObject> future = fn.invoker().arg(4).arg(5).executeAsync();
    PyObject result = future.get(10, TimeUnit.SECONDS);
    assertEquals(result.toString(), "9");
  }

  @Test(timeOut = 15000)
  public void testExecuteAsyncWithTimeoutSucceeds() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("lambda: 42");
    Future<PyObject> future = fn.invoker().executeAsync(5000);
    PyObject result = future.get(10, TimeUnit.SECONDS);
    assertEquals(result.toString(), "42");
  }

  @Test(timeOut = 15000, expectedExceptions = {TimeoutException.class, ExecutionException.class})
  public void testExecuteAsyncWithTimeoutExpires() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("__import__('time').sleep");
    Future<PyObject> future = fn.invoker().arg(2.0).executeAsync(200);
    future.get(10, TimeUnit.SECONDS);
  }

  @Test
  public void testCallBuilderEntry()
  {
    CallBuilderEntry<String, Integer> entry = new CallBuilderEntry<>("k", 42);
    assertEquals(entry.getKey(), "k");
    assertEquals(entry.getValue(), Integer.valueOf(42));
    assertThrows(UnsupportedOperationException.class, () -> entry.setValue(7));
  }
}
