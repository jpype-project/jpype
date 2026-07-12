// --- file: org/jpype/SubInterpreter.java ---
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

import java.util.logging.Level;
import java.util.logging.Logger;
import org.jpype.internal.NativeContext;
import org.jpype.internal.NativeLauncherControl;
import python.lang.PyBuiltIn;
import python.lang.PyObject;

/**
 * A PEP 684 subinterpreter running alongside {@link MainInterpreter} (or
 * another {@code SubInterpreter}) in the same JVM.
 *
 * <p>
 * Unlike {@link MainInterpreter}, this class is not a singleton. Each
 * instance owns its own Python interpreter, its own GIL, and its own set of
 * Java-side bindings (backend, installer, builtin) as per-instance fields
 * rather than statics, so multiple instances may be started and closed
 * independently without clobbering each other's state.</p>
 *
 * <p>
 * Requires Python 3.12+ (built with PEP 684 support); {@link #start} throws
 * on older interpreters.</p>
 */
public class SubInterpreter implements Interpreter
{

  final static Logger LOGGER = Logger.getLogger(SubInterpreter.class.getName());

  private NativeContext context;
  private Backend backend;
  private Installer installer;
  private PyBuiltIn builtin;
  private boolean terminated = false;

  /**
   * Python object used to signal interpreter shutdown.
   */
  public PyObject stop = null;

  /**
   * Returns the backend used to interact with Python.
   *
   * @return The {@link Backend} instance.
   */
  public Backend getBackend()
  {
    return backend;
  }

  /**
   * Sets the backend used to interact with Python.
   *
   * <p>
   * Called back from {@code _jbridge.py} during {@link #start}, once the
   * subinterpreter's own {@code _jpype} module has finished initializing.</p>
   *
   * @param ctx
   * @param entry The {@link Backend} instance to set.
   */
  public void setBackend(NativeContext ctx, Backend entry)
  {
    if (backend != null)
      throw new RuntimeException("Backend reconfigured");
    LOGGER.log(Level.INFO, "Backend installed");
    backend = entry;
    this.context = ctx;
    this.builtin = new InterpreterBuiltIn(ctx, backend);
    stop = backend.object();
    ctx.setBuiltIn(this.builtin);
  }

  /**
   * Returns the SPI installer used to register Python-class/interface
   * bindings and provider mini-backends.
   *
   * @return The {@link Installer} instance.
   */
  public Installer getInstaller()
  {
    return installer;
  }

  /**
   * Sets the SPI installer and immediately drives eager SPI registration
   * ({@link SpiLoader#load}) against this subinterpreter's own
   * {@link Installer} instance (not shared with {@link MainInterpreter} or
   * any other subinterpreter).
   *
   * @param installer The {@link Installer} instance to set.
   */
  public void setInstaller(Installer installer)
  {
    if (this.installer != null)
      throw new RuntimeException("Installer reconfigured");
    LOGGER.log(Level.INFO, "Installer installed");
    this.installer = installer;
    SpiLoader.load(installer);
  }

  @Override
  public PyBuiltIn getBuiltIn()
  {
    return this.builtin;
  }

  public boolean isStarted()
  {
    return backend != null;
  }

  /**
   * Starts the subinterpreter with the legacy configuration (shared GIL,
   * shared obmalloc - the same fixed values this method always used).
   *
   * <p>
   * {@link MainInterpreter#start} must already have loaded the native
   * libraries (this call reuses the process-wide Python/JPype native
   * libraries loaded by the main interpreter; it does not load them
   * again).</p>
   *
   * @see org.jpype.SubInterpreterBuilder for a configurable alternative
   * (own-GIL isolation, allow_fork/exec/threads, etc.).
   */
  public synchronized void start()
  {
    start(true, false, false, true, false, false, false);
  }

  /**
   * Starts the subinterpreter with an explicit {@code PyInterpreterConfig}.
   *
   * Package-private: external callers should go through
   * {@link SubInterpreterBuilder}, which validates the legal combinations
   * of these flags before calling here.
   */
  synchronized void start(boolean useMainObmalloc, boolean allowFork, boolean allowExec,
          boolean allowThreads, boolean allowDaemonThreads,
          boolean checkMultiInterpExtensions, boolean ownGil)
  {
    if (terminated)
      throw new IllegalStateException("interpreter is terminated");
    if (backend != null)
      return;

    this.context = NativeLauncherControl.startSubInterpreter(this,
            useMainObmalloc, allowFork, allowExec, allowThreads,
            allowDaemonThreads, checkMultiInterpExtensions, ownGil);

    // Control is back on the Java thread here. Confirm the launch didn't
    // leave the subinterpreter's GIL held on the calling thread.
    if (NativeLauncherControl.isGilHeld())
    {
      LOGGER.severe("GIL still held on calling thread after subinterpreter launch - leaked GIL acquire during startup.");
    }
  }

  /**
   * Closes the subinterpreter.
   *
   * This is irrevokable.
   */
  @Override
  public synchronized void close()
  {
    if (terminated)
      throw new IllegalStateException("interpreter is terminated");

    NativeLauncherControl.finishSub(this.context.address());
    backend = null;
    terminated = true;
  }
}
