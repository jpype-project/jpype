// --- file: python/lang/PyKwArgsNGTest.java ---
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
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

/**
 * Exercises keyword-argument support on the reverse proxy bridge (Java
 * calling into a Python-implemented interface): {@link PyKwArgs} appended
 * as the trailing element of a varargs call, flattened and passed through
 * {@code ProxyInstance.invoke()}/{@code hostInvoke} as {@code **kwargs}.
 */
public class PyKwArgsNGTest extends PyTestHarness
{

  @BeforeMethod
  public void registerCapture()
  {
    // A fresh Python closure per test so leftover state can't leak between
    // tests sharing the single KwCallRegistry.captured slot.
    context.exec(
            "import jpype\n"
            + "_KwCallRegistry = jpype.JClass('python.lang.KwCallRegistry')\n"
            + "def _kwfn(*args, **kwargs):\n"
            + "    return (args, kwargs)\n"
            + "_KwCallRegistry.register(_kwfn)\n"
    );
  }

  @Test
  public void testKeywordArgumentsPassThroughProxy()
  {
    PyObject result = KwCallRegistry.captured.call(1, 2, "three",
            PyKwArgs.of().kw("verbose", true).kw("limit", 10));
    assertEquals(result.toString(),
            "((1, 2, 'three'), {'verbose': True, 'limit': 10})");
  }

  @Test
  public void testNoKeywordArgumentsWhenNoPyKwArgsMarker()
  {
    PyObject result = KwCallRegistry.captured.call(1, 2, "three");
    assertEquals(result.toString(),
            "((1, 2, 'three'), {})");
  }

  @Test
  public void testEmptyPyKwArgsMarkerIsDroppedNotPassedPositionally()
  {
    PyObject result = KwCallRegistry.captured.call(1, 2, PyKwArgs.of());
    assertEquals(result.toString(),
            "((1, 2), {})");
  }

  @Test
  public void testOnlyKeywordArguments()
  {
    PyObject result = KwCallRegistry.captured.call(
            PyKwArgs.of().kw("a", 1).kw("b", 2));
    assertEquals(result.toString(),
            "((), {'a': 1, 'b': 2})");
  }
}
