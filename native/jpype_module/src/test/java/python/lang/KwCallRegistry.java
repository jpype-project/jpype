// --- file: python/lang/KwCallRegistry.java ---
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

/**
 * Test-only support class for {@link PyKwArgsNGTest}.
 *
 * A Python lambda is registered here from Python source executed by the
 * test (via {@code jpype.JClass(...).register(...)}), which forces JPype's
 * automatic functional-interface conversion to wrap it as a {@link KwCall}
 * proxy - the same reverse-bridge machinery (ProxyInstance/hostInvoke) used
 * by any Python object implementing a Java interface. The test then calls
 * {@link #captured} directly from Java to exercise varargs + PyKwArgs
 * handling.
 */
public class KwCallRegistry
{

  public static KwCall captured;

  public static void register(KwCall c)
  {
    captured = c;
  }

  @FunctionalInterface
  public interface KwCall
  {
    PyObject call(Object... args);
  }
}
