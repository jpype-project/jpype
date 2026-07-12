// --- file: org/jpype/Installer.java ---
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
package org.jpype;

/**
 * Internal: registers a {@link WrapperService} provider's Python classes
 * and mini-backends with the running interpreter. Implemented by
 * {@code _jbridge.py} and installed on {@link MainInterpreter} at startup;
 * a provider never constructs or calls this type directly — its methods
 * are invoked automatically, once per declared resource, after
 * {@link WrapperService#getResources()} runs.
 */
public interface Installer
{

  /**
   * Register a concrete Python class as satisfying a Java interface.
   *
   * @param pyModule the Python module the class lives in, e.g. {@code "_io"}
   * (matching the class's real {@code __module__} — not necessarily the
   * "friendly" package name a provider's own module list reports).
   * @param pyClass the class's name within that module, e.g. {@code
   * "BytesIO"}.
   * @param javaInterface fully qualified Java interface name, e.g.
   * {@code "python.io.PyBytesIO"}.
   * @param methodsSource Python source text; when {@code exec}'d, must bind
   * a top-level name {@code METHODS} to a {@code dict[str, Callable]} in the
   * same shape as the hand-written {@code _PyXxxMethods} dicts this
   * replaces.
   */
  void registerClass(String pyModule, String pyClass, String javaInterface, String methodsSource);

  /**
   * Record a class registration to be replayed lazily, the first time a
   * Python instance of {@code pyClass} is actually seen crossing into Java
   * (via the {@code _jpype._cache} probe-miss hook), rather than
   * immediately. Unlike {@link #registerClass}, this must not import
   * {@code pyModule} or {@code exec} {@code methodsSource} — it only
   * stores the ingredients for later.
   *
   * @param pyModule the Python module the class lives in, same convention
   * as {@link #registerClass}.
   * @param pyClass the class's name within that module.
   * @param javaInterface fully qualified Java interface name.
   * @param methodsSource Python source text; deferred {@code exec} target,
   * same contract as {@link #registerClass}.
   */
  void registerLazyClass(String pyModule, String pyClass, String javaInterface, String methodsSource);

  /**
   * Register a provider's own mini-backend — a small, provider-owned
   * interface (like {@code python.io.IO}) that needs its own
   * {@code JProxy} and dispatch dict, independent of the shared
   * {@link Backend}, which providers cannot extend.
   *
   * @param javaInterface fully qualified Java interface name for the
   * mini-backend, e.g. {@code "python.io.IO"}.
   * @param methodsSource Python source text; when {@code exec}'d, must bind
   * a top-level name {@code METHODS} to a {@code dict[str, Callable]}.
   */
  void registerBackend(String javaInterface, String methodsSource);

}
