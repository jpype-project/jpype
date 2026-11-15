// --- file: python/lang/PyAliceBobCharlieDerik.java ---
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
 * Test-only fixture interface for {@code plan/DispatchFallback.md}: proves
 * that {@code $}-prefixed methods on a {@link PyObject}-rooted interface
 * fall through the mangled dispatch map to a real Python object's own
 * attributes, with overload selection, {@link PyKwArgs}-based keyword
 * calls, and varargs all landing the correct Python call shape.
 */
public interface PyAliceBobCharlieDerik extends PyObject
{
  PyTuple $alice(Object arg);

  PyTuple $bob(Object a, Object b);

  PyTuple $bob(Object a, Object b, PyKwArgs kw);

  PyTuple $charlie(Object... args);

  PyTuple $derik(PyKwArgs kw);

  // --- Adversarial edge cases (plan/DispatchFallback.md "hazard" pass) ---

  /** Real attribute, but calling it always raises on the Python side. */
  PyTuple $hazel();

  /** No matching Python attribute at all - a total dispatch miss. */
  PyTuple $ghost();

  /** Real, callable attribute, but its return value isn't a tuple. */
  PyTuple $ike();

  /** Real attribute, but it isn't callable at all (a plain int). */
  PyTuple $wilma();
}
