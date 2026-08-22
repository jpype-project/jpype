// --- file: python/lang/PyKwArgs.java ---
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

/**
 * Marks a trailing argument as keyword arguments for a reverse-proxy call.
 *
 * When Java code invokes a varargs method on a Python-backed proxy (a
 * Python object implementing a Java interface declared as
 * {@code Object someMethod(Object... args)}), appending a {@code PyKwArgs}
 * as the last element signals that its entries should be passed to the
 * underlying Python callable as {@code **kwargs} rather than as a positional
 * argument. {@link org.jpype.proxy.ProxyInstance} recognizes and strips this
 * marker before the call crosses into native code.
 *
 * <p>
 * Usage example:</p>
 * <pre>
 * proxy.someMethod(1, 2, PyKwArgs.of().kw("verbose", true).kw("limit", 10));
 * </pre>
 */
public final class PyKwArgs extends LinkedHashMap<String, Object>
{

  /**
   * Creates a new, empty {@code PyKwArgs}.
   *
   * @return a new {@code PyKwArgs} instance.
   */
  public static PyKwArgs of()
  {
    return new PyKwArgs();
  }

  /**
   * Adds a single keyword argument.
   *
   * @param name is the keyword argument name.
   * @param value is the keyword argument value.
   * @return this instance for chaining.
   */
  public PyKwArgs kw(String name, Object value)
  {
    put(name, value);
    return this;
  }
}
