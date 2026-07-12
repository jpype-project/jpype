// --- file: org/jpype/internal/NativeLauncherControl.java ---
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
package org.jpype.internal;

/**
 * Internal behaviors used by the binding.
 *
 * Loaded by the _jpype module.
 *
 */
public class NativeLauncherControl
{

  public native static NativeContext startMain(String[] modulePaths, String[] args,
          String name, String prefix, String home, String exec_prefix, String executable,
          boolean isolated, boolean fault_handler, boolean quiet, boolean verbose,
          boolean site_import, boolean user_site, boolean write_bytecode, Object interpreter);

  public native static void interactive(long context);

  public native static void finishMain(long context);

  /**
   * Create a new PEP 684 subinterpreter with its own GIL and attach a fresh
   * JPContext to it.
   *
   * Requires Python 3.12+ (older builds throw immediately). The calling
   * thread does not hold the subinterpreter's GIL on return - control comes
   * back to Java exactly as it does after {@link #startMain}.
   *
   * The seven boolean parameters map directly onto CPython's
   * {@code PyInterpreterConfig} fields of the same name (with
   * {@code ownGil} selecting {@code PyInterpreterConfig_OWN_GIL} vs.
   * {@code PyInterpreterConfig_DEFAULT_GIL}). Callers should go through
   * {@code org.jpype.SubInterpreterBuilder} rather than choosing these
   * directly - it validates the one hard cross-field CPython rule
   * (use_main_obmalloc=0 requires check_multi_interp_extensions=1) before
   * this native call is ever reached.
   *
   * @param interpreter the {@link org.jpype.Interpreter} instance to bind as
   * the subinterpreter's back-reference (returned by {@code _jpype.interpreter()}
   * on the Python side of that subinterpreter).
   * @return the {@link NativeContext} for the new subinterpreter.
   */
  public native static NativeContext startSubInterpreter(Object interpreter,
          boolean useMainObmalloc, boolean allowFork, boolean allowExec,
          boolean allowThreads, boolean allowDaemonThreads,
          boolean checkMultiInterpExtensions, boolean ownGil);

  /**
   * Tear down a subinterpreter previously created with
   * {@link #startSubInterpreter}.
   *
   * @param context the address of the subinterpreter's {@code JPContext}
   * (see {@link NativeContext#address()}).
   */
  public native static void finishSub(long context);

  /**
   * Check whether the calling thread currently holds the Python GIL.
   *
   * This is intended to be asserted from Java code at points where the
   * calling thread is guaranteed to have already returned control to Java
   * (e.g. after any bridge call, or in test teardown) so that a leaked or
   * double-released GIL hold on some invisible, ungrepped code path can be
   * caught close to its source rather than surfacing later as a hang.
   *
   * Safe to call whether or not the calling thread currently holds the GIL.
   */
  public native static boolean isGilHeld();

}
