// --- file: python/io/PyLazySpiNGTest.java ---
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
package python.io;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyObject;
import python.lang.PyString;
import python.lang.PyTestHarness;

/**
 * Proves the lazy SPI hook (see {@code plan/SPI.md}) fires with no
 * explicit invocation: {@code _io.StringIO} is registered under
 * {@code python.io.PyIOWrapperService#getResources()} with
 * {@code lazy: true}, so nothing on the Java side ever calls
 * {@code IO.using(context).stringIO()} here - a raw Python value built
 * entirely on the Python side is cast straight to {@link PyStringIO}, and
 * the {@code _jpype._cache} probe-miss hook must resolve it on its own the
 * first time it crosses into Java.
 */
public class PyLazySpiNGTest extends PyTestHarness
{

  @Test
  public void testRawPythonStringIOResolvesLazily()
  {
    PyObject raw = context.eval("__import__('io').StringIO('hello')");

    PyStringIO instance = (PyStringIO) raw;

    assertNotNull(instance);
    PyString value = instance.getvalue();
    assertEquals(value.toString(), "hello");
  }

  @Test
  public void testLazilyResolvedInstanceBehavesNormally()
  {
    PyStringIO instance = (PyStringIO) context.eval("__import__('io').StringIO('abcdef')");

    instance.seek(2);

    assertEquals(instance.tell(), 2L);
    assertEquals(instance.read().toString(), "cdef");
  }
}
