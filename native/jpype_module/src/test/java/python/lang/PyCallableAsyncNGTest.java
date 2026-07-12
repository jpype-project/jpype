// --- file: python/lang/PyCallableAsyncNGTest.java ---
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

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyCallableAsyncNGTest extends PyTestHarness
{

  @Test(timeOut = 15000)
  public void testCallAsyncReturnsResult() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("lambda x, y: x + y");
    Future<PyObject> future = fn.callAsync(context.tuple(2, 3), context.dict());
    PyObject result = future.get(10, TimeUnit.SECONDS);
    assertEquals(result.toString(), "5");
  }

  @Test(timeOut = 15000)
  public void testManyConcurrentCallAsync() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("lambda x: x * x");
    Future<?>[] futures = new Future<?>[50];
    for (int i = 0; i < futures.length; i++)
      futures[i] = fn.callAsync(context.tuple(i), context.dict());
    for (int i = 0; i < futures.length; i++)
    {
      PyObject result = (PyObject) futures[i].get(10, TimeUnit.SECONDS);
      assertEquals(result.toString(), String.valueOf(i * i));
    }
  }

  @Test(timeOut = 15000)
  public void testCallAsyncWithTimeoutSucceeds() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("lambda: 42");
    Future<PyObject> future = fn.callAsyncWithTimeout(context.tuple(), context.dict(), 5000);
    PyObject result = future.get(10, TimeUnit.SECONDS);
    assertEquals(result.toString(), "42");
  }

  @Test(timeOut = 15000, expectedExceptions = {TimeoutException.class, ExecutionException.class})
  public void testCallAsyncWithTimeoutExpires() throws Exception
  {
    PyCallable fn = (PyCallable) context.eval("__import__('time').sleep");
    Future<PyObject> future = fn.callAsyncWithTimeout(context.tuple(2.0), context.dict(), 200);
    future.get(10, TimeUnit.SECONDS);
  }

}
