// --- file: org/jpype/MainInterpreter.java ---
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

import org.jpype.internal.NativeLauncherControl;
import org.jpype.internal.NativeContext;
import java.io.File;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import python.lang.PyBuiltIn;
import python.lang.PyCallable;
import python.lang.PyObject;

/**
 * Frontend for managing the Python interpreter within JPype.
 *
 * <p>
 * This class serves as the main entry point for interacting with the Python
 * interpreter. It is a singleton that is created once to connect to Python. To
 * start the interpreter, configure all necessary variables and then call
 * {@link #start(String[])}.</p>
 *
 * <p>
 * The {@code Interpreter} class provides methods for locating the Python
 * executable, probing Python installations, and managing Python module paths.
 * It also allows applications to operate interactively with the Python
 * interpreter.</p>
 *
 *
 * To log use: java -Djava.util.logging.config.file=logging.properties -cp
 * your_app.jar com.your.Main
 *
 * With logging.properties file:
 * <pre>
 *   handlers=java.util.logging.ConsoleHandler
 *   org.jpype.MainInterpreter.level=INFO
 *   java.util.logging.ConsoleHandler.level=INFO
 *   java.util.logging.ConsoleHandler.formatter=java.util.logging.SimpleFormatter
 * </pre>
 *
 */
public class MainInterpreter implements Interpreter
{

  final static Logger LOGGER = Logger.getLogger(NativeContext.class.getName());

  // Configuration variables
  final static String CONF_NAME = "python.config.program_name";
  final static String CONF_HOME = "python.config.home";
  final static String CONF_PATH = "python.config.path";
  final static String CONF_EXECUTABLE = "python.config.executable";
  final static String CONF_ISOLATED = "python.config.isolated";
  final static String CONF_FAULTHANDLER = "python.config.fault_handler";
  final static String CONF_QUIET = "python.config.quiet";
  final static String CONF_VERBOSE = "python.config.verbose";
  final static String CONF_SITEIMPORT = "python.config.site_import";
  final static String CONF_USERSITE = "python.config.user_site_directory";
  final static String CONF_WRITEBC = "python.config.write_bytecode";

  final static String MOD_PATH = "python.module.path";

  /**
   * Indicates whether the interpreter is active.
   */
  private boolean active = false;
  private PyBuiltIn builtin;
  private NativeContext context;
  private boolean terminated = false;

  /**
   * Locates, probes, and loads the Python environment this interpreter runs.
   */
  private final Launcher launcher = new Launcher();

  /**
   * List of module paths used by the Python interpreter.
   */
  private final List<String> modulePaths = new ArrayList<>();

  /**
   * The backend used to interact with Python.
   */
  static Backend backend = null;

  /**
   * The SPI installer implemented by {@code _jbridge.py}. See
   * {@link Installer}, {@link SpiLoader}.
   */
  static Installer installer = null;

  /**
   * Singleton instance of the {@code Interpreter}.
   */
  static MainInterpreter INSTANCE = new MainInterpreter();

  /**
   * Python object used to signal interpreter shutdown.
   */
  public static PyObject stop = null;

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
   * This method is called during the initialization process to establish the
   * backend connection.</p>
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
   * ({@link SpiLoader#load}) — every {@code WrapperService} discovered via
   * {@code ServiceLoader} gets its declared {@code .pyspi} resources read
   * and replayed into it. Called once from {@code _jbridge.py}'s
   * {@code initialize()}, after {@link #setBackend} (SPI-registered classes
   * may need the backend to already exist).
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
     
  /**
   * Returns the singleton instance of the {@code Interpreter}.
   *
   * @return The singleton {@code Interpreter} instance.
   */
  public static MainInterpreter getInstance()
  {
    return INSTANCE;
  }

  /**
   * Main entry point for starting the Python interpreter.
   * <p>
   * This method initializes the interpreter, starts it with the provided
   * arguments, and enters interactive mode.</p>
   *
   * @param args Command-line arguments passed to the interpreter.
   */
  public static void main(String[] args)
  {
    // This will be the entry point for pretending we are a python variant and
    // launching a python shell.
    MainInterpreter interpreter = getInstance();
    interpreter.start(args);
    interpreter.dispatch(args);
  }

  /**
   * Interprets {@code args} the way the real {@code python} command line
   * would - {@code -c command}, {@code -m module}, a bare script-file
   * argument, an optional leading {@code -i} to drop into the interactive
   * shell afterward, or (with none of the above) go straight to
   * {@link #interactive()}, matching today's default behavior.
   *
   * <p>
   * Built on {@link Runner}, so the actual execution is exactly
   * {@code runCommand}/{@code runModule}/{@code runFile} - see
   * {@code plan/PythonCLI.md} for why this isn't done via {@code Py_RunMain}
   * or by reading {@code PyConfig}'s own already-parsed argv fields.</p>
   *
   * @param args command-line arguments, as passed to {@link #main(String[])}.
   */
  public void dispatch(String[] args)
  {
    int i = 0;
    boolean interactiveAfter = false;
    while (i < args.length && args[i].equals("-i"))
    {
      interactiveAfter = true;
      i++;
    }
    if (i >= args.length)
    {
      interactive();
      return;
    }

    String token = args[i];
    String[] remaining = Arrays.copyOfRange(args, i + 1, args.length);
    Runner runner = new Runner(this);
    if (token.equals("-c"))
    {
      if (remaining.length == 0)
        throw new IllegalArgumentException("Argument expected for the -c option");
      runner.runCommand(remaining[0], Arrays.copyOfRange(remaining, 1, remaining.length));
    } else if (token.equals("-m"))
    {
      if (remaining.length == 0)
        throw new IllegalArgumentException("Argument expected for the -m option");
      runner.runModule(remaining[0], Arrays.copyOfRange(remaining, 1, remaining.length));
    } else
    {
      runner.runFile(Paths.get(token), remaining);
    }

    if (interactiveAfter)
      interactive();
  }

  /**
   * Returns the list of module paths used by the Python interpreter.
   *
   * <p>
   * This list can be used to limit the modules available for embedded
   * applications. If the list is not empty, the default module path will not be
   * used.</p>
   *
   * @return The list of module paths.
   */
  public List<String> getModulePaths()
  {
    return modulePaths;
  }

  /**
   * Enters interactive mode with the Python interpreter.
   *
   * <p>
   * This method allows the user to interact directly with the Python
   * interpreter.</p>
   */
  public void interactive()
  {
    if (!isStarted())
      throw new IllegalStateException("Python interpreter not running");
    NativeLauncherControl.interactive(context.address());
  }

  /**
   * Get the method used to start the interpreter.
   *
   * The interpreter may have been started from either Java or Python. If
   * started from Java side we clean up resources differently, becuase Python
   * shuts down before Java in that case.
   *
   * @return true if the interpreter was started from Java.
   */
  public boolean isJava()
  {
    return active;
  }

  public boolean isStarted()
  {
    return backend != null;
  }

  /**
   * Prepares the interpreter environment without launching.
   * <p>
   * This method runs the detective probe (or loads from cache), identifies the
   * correct libraries, and performs self-healing if necessary. After calling
   * this, you can inspect the System properties starting with 'python.config.'
   * or 'jpype.' before calling {@link #start(String...)}.
   * </p>
   */
  public void prepare()
  {
    launcher.prepare();
    LOGGER.info("Interpreter prepared. Ready for inspection or launch.");
  }

  /**
   * Start the interpreter.Any configuration actions must have been completed
   * before the interpreter is started.
   *
   * Many configuration variables may be adjusted with Java System properties.
   *
   * @param args
   */
  public synchronized void start(String... args)
  {
    if (terminated)
      throw new IllegalStateException("interpreter is terminated");

    if (MainInterpreter.backend != null || active)
      return;
    active = true;

    // 1. Ensure the environment is prepared
    if (!launcher.isPrepared())
      prepare();

    // 2. Audit check: Ensure we have the minimum viable components
    if (launcher.getJpypeLibrary() == null || launcher.getPythonLibrary() == null)
    {
      LOGGER.severe("Bridge initialization failed: Libraries not found.");
      throw new UnsatisfiedLinkError("Unable to find _jpype or libpython modules");
    }

    if (launcher.getJpypeVersion().equals("NOT_FOUND"))
      throw new UnsatisfiedLinkError("Jpype module not found");

    // 4. Compatibility check
    int[] version = Launcher.parseVersion(launcher.getJpypeVersion());
    int[] required = Launcher.parseVersion(NativeContext.VERSION);
    if (version[0] < required[0] || (version[0] == required[0] && version[1] < required[1]))
      throw new LinkageError("JPype version " + launcher.getJpypeVersion() + " is older than required " + NativeContext.VERSION);

    // 5. Load Native Binaries
    launcher.installNatives();

    // 6. Final Configuration Dump (The "Detective's" Report)
    // Pull everything from system properties now that the launcher has run
    String programName = System.getProperty(CONF_NAME, launcher.getPythonExecutable());
    String home = System.getProperty(CONF_HOME);
    String pythonPath = System.getProperty(CONF_PATH);

    LOGGER.info("Python C-API Configuration:");
    LOGGER.log(Level.INFO, "  program_name: {0}", programName);
    LOGGER.log(Level.INFO, "  home:         {0}", home);
    LOGGER.log(Level.INFO, "  pythonpath:   {0}", pythonPath);
    LOGGER.log(Level.INFO, "  isolated:     {0}", System.getProperty(CONF_ISOLATED, "false"));
    LOGGER.log(Level.INFO, "  site_import:  {0}", System.getProperty(CONF_SITEIMPORT, "true"));

    // Prepare paths
    List<String> allPaths = new ArrayList<>(this.modulePaths);
    String sysPythonPath = System.getProperty(CONF_PATH);
    if (sysPythonPath != null)
    {
      // Split by system path separator (':' on Linux, ';' on Windows)
      allPaths.addAll(Arrays.asList(sysPythonPath.split(File.pathSeparator)));
    }
    String[] paths = allPaths.toArray(new String[0]);

    LOGGER.log(Level.FINE, "Module paths");
    for (String path : paths)
    {
      LOGGER.log(Level.FINE, "  {0}", path);
    }

    // 7. Launch
    LOGGER.info("Launching Python Interpreter...");
    this.context = NativeLauncherControl.startMain(paths, args,
            programName,
            home,
            System.getProperty(CONF_EXECUTABLE, launcher.getPythonExecutable()),
            getBool(CONF_ISOLATED, "false"),
            getBool(CONF_FAULTHANDLER, "false"),
            getBool(CONF_QUIET, "false"),
            getBool(CONF_VERBOSE, "false"),
            getBool(CONF_SITEIMPORT, "true"),
            getBool(CONF_USERSITE, "true"),
            getBool(CONF_WRITEBC, "true"),
            this,
            launcher.getJpypeLibrary());
    LOGGER.info("Python launched");

    // Control is back on the Java thread here. Confirm the launch didn't
    // leave the GIL held - a leak on this path would be invisible to grep
    // and would only otherwise surface later as a hang on some other thread.
    if (NativeLauncherControl.isGilHeld())
    {
      LOGGER.severe("GIL still held on calling thread after Python interpreter launch - leaked GIL acquire during startup.");
    }
  }

  /**
   * Close the bridge.
   *
   * This is irrevokable.
   */
  @Override
  public synchronized void close()
  {
    if (terminated)
      throw new IllegalStateException("interpreter is terminated");

    // Stop (and join) the reference-queue daemon thread before Py_Finalize()
    // runs inside finishMain(). Without this, that thread can still be
    // processing GC'd phantom refs - and therefore still calling into the
    // Python C API - concurrently with Py_Finalize() tearing down the
    // interpreter's thread-state machinery on this thread. The JVM-shutdown-
    // hook path (NativeContext.shutdown()) also calls stop() later as a
    // backstop for resources that outlive an explicit close(); stop() must
    // therefore tolerate being called twice.
    this.context.getReferenceQueue().stop();

    // Same hazard as the reference queue, via a second unguarded background
    // thread: ASYNC_POOL workers can still be mid-call into the Python C API
    // (e.g. a callAsyncWithTimeout() whose Future.cancel(true) fired but
    // couldn't actually interrupt a thread blocked in native code) when
    // Py_Finalize() runs below. Stop accepting new work and wait for
    // in-flight calls to finish naturally before finalizing.
    PyCallable.ASYNC_POOL.shutdown();
    try
    {
      if (!PyCallable.ASYNC_POOL.awaitTermination(10, java.util.concurrent.TimeUnit.SECONDS))
        LOGGER.severe("ASYNC_POOL did not quiesce before interpreter shutdown - "
                + "an async call may still be running against a finalizing interpreter.");
    } catch (InterruptedException ex)
    {
      Thread.currentThread().interrupt();
    }

    NativeLauncherControl.finishMain(this.context.address());
    backend = null;
    terminated = true;
  }

//<editor-fold desc="internal" defaultstate="collapsed">
  /**
   * Constructs a new {@code Interpreter}.
   *
   * <p>
   * This constructor initializes the interpreter and sets up any module paths
   * specified via the Java system property {@code python.module.path}.</p>
   */
  private MainInterpreter()
  {
    String paths = System.getProperty(MOD_PATH);
    if (paths != null)
      this.modulePaths.addAll(Arrays.asList(paths.split(File.pathSeparator)));
  }

  private boolean getBool(String key, String def)
  {
    return Boolean.parseBoolean(System.getProperty(key, def));
  }

//</editor-fold>
}
