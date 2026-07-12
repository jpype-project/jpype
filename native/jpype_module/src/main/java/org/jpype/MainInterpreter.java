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
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Stream;
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
  final static String PROBE = "/org/jpype/resources/probe.py";

  // Configuration variables
  final static String CONF_NAME = "python.config.program_name";
  final static String CONF_HOME = "python.config.home";
  final static String CONF_PATH = "python.config.path";
  final static String CONF_PREFIX = "python.config.prefix";
  final static String CONF_EXEPREFIX = "python.config.exec_prefix";
  final static String CONF_EXECUTABLE = "python.config.executable";
  final static String CONF_ISOLATED = "python.config.isolated";
  final static String CONF_FAULTHANDLER = "python.config.fault_handler";
  final static String CONF_QUIET = "python.config.quiet";
  final static String CONF_VERBOSE = "python.config.verbose";
  final static String CONF_SITEIMPORT = "python.config.site_import";
  final static String CONF_USERSITE = "python.config.user_site_directory";
  final static String CONF_WRITEBC = "python.config.write_bytecode";

  final static String MOD_PATH = "python.module.path";
  final static String PYTHON_EXEC = "python.executable";
  final static String PYTHON_LIB = "python.lib";

  final static String JPYPE_LIB = "jpype.lib";
  final static String JPYPE_ARCH = "jpype.arch";
  final static String JPYPE_NOCACHE = "jpype.nocache";
  final static String JPYPE_INSTALL = "jpype.install";
  final static String JPYPE_VER = "jpype.version";

  final static String PROPERTIES = "jpype.properties";

  /**
   * Indicates whether the interpreter is active.
   */
  private boolean active = false;
  private PyBuiltIn builtin;
  private NativeContext context;
  private boolean terminated = false;

  /**
   * Indicates whether the operating system is Windows.
   */
  private final boolean isWindows = checkWindows();

  /**
   * Path to the JPype library.
   */
  private String jpypeLibrary;

  /**
   * Version of the JPype library.
   */
  private String jpypeVersion;

  /**
   * List of module paths used by the Python interpreter.
   */
  private final List<String> modulePaths = new ArrayList<>();
  private String pythonExecutable;

  /**
   * Path to the Python library.
   */
  private String pythonLibrary;

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
    interpreter.interactive();
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
    if (this.pythonLibrary != null)
      return; // Already prepared or started

    // Run the environmental audit
    resolveLibraries();

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
    if (this.pythonLibrary == null)
      prepare();

    // 2. Audit check: Ensure we have the minimum viable components
    if (jpypeLibrary == null || pythonLibrary == null)
    {
      LOGGER.severe("Bridge initialization failed: Libraries not found.");
      throw new UnsatisfiedLinkError("Unable to find _jpype or libpython modules");
    }

    if (this.jpypeVersion.equals("NOT_FOUND"))
      throw new UnsatisfiedLinkError("Jpype module not found");

    // 4. Compatibility check
    int[] version = parseVersion(this.jpypeVersion);
    int[] required = parseVersion(NativeContext.VERSION);
    if (version[0] < required[0] || (version[0] == required[0] && version[1] < required[1]))
      throw new LinkageError("JPype version " + jpypeVersion + " is older than required " + NativeContext.VERSION);

    // 5. Load Native Binaries
    installNatives();

    // 6. Final Configuration Dump (The "Detective's" Report)
    // Pull everything from system properties now that resolveLibraries() has run
    String programName = System.getProperty(CONF_NAME, this.pythonExecutable);
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
            System.getProperty(CONF_PREFIX),
            home,
            System.getProperty(CONF_EXEPREFIX),
            System.getProperty(CONF_EXECUTABLE, this.pythonExecutable),
            getBool(CONF_ISOLATED, "false"),
            getBool(CONF_FAULTHANDLER, "false"),
            getBool(CONF_QUIET, "false"),
            getBool(CONF_VERBOSE, "false"),
            getBool(CONF_SITEIMPORT, "true"),
            getBool(CONF_USERSITE, "true"),
            getBool(CONF_WRITEBC, "true"),
            this);
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
   * Determines if the current operating system is Windows.
   *
   * @return {@code true} if the operating system is Windows; {@code false}
   * otherwise.
   */
  private static boolean checkWindows()
  {
    String osName = System.getProperty("os.name");
    return osName.startsWith("Windows");
  }

  /**
   * Loads the detective probe from the resources directory.
   */
  private String loadProbeResource()
  {
    // Get the module that contains the Interpreter class
    Module module = MainInterpreter.class.getModule();

    try (InputStream is = module.getResourceAsStream(PROBE))
    {
      if (is == null)
      {
        throw new RuntimeException("Missing resource: " + PROBE
                + " (Ensure it is in the module path and the package is included in the JAR)");
      }
      return new String(is.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
    } catch (IOException e)
    {
      throw new RuntimeException("Failed to load detective probe from resources", e);
    }
  }

  /**
   * Parses a version string into an integer array.
   *
   * @param version The version string to parse (e.g., "3.9.7").
   * @return An integer array representing the version (e.g., [3, 9, 7]).
   */
  private static int[] parseVersion(String version)
  {
    String[] parts = version.split("\\.");
    int[] out = new int[3];
    try
    {
      for (int i = 0; i < parts.length; ++i)
      {
        if (i == 3)
          break;
        out[i] = Integer.parseInt(parts[i]);
      }
    } catch (NumberFormatException ex)
    {
    }
    return out;
  }

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

  /**
   * Searches the system PATH for an executable.
   *
   * @param exec The name of the executable to search for.
   * @return The path to the executable, or {@code null} if not found.
   */
  private String checkPath(String exec)
  {
    String path = System.getenv("PATH");
    if (path == null)
      return null;
    String[] parts = path.split(File.pathSeparator);
    for (String part : parts)
    {
      Path test = Paths.get(part, exec);
      if (Files.exists(test) && Files.isExecutable(test))
        return test.toString();
    }
    return null;
  }

  /**
   * Determines the location of the Python executable to use for probing.
   *
   * <p>
   * This method uses the following sources to locate the executable:</p>
   * <ul>
   * <li>Java system property {@code python.executable}</li>
   * <li>Environment variable {@code PYTHONHOME}</li>
   * <li>First `python3` found in the system PATH</li>
   * </ul>
   *
   * @return The path to the Python executable.
   * @throws RuntimeException If the executable cannot be located.
   */
  private String getExecutable()
  {
    // Was is supplied via Java?
    String out = System.getProperty(PYTHON_EXEC);

    // Was it passed as environment variable?
    if (out != null)
      return out;

    String home = System.getenv("PYTHONHOME");
    if (home != null)
    {
      if (isWindows)
      {
        return Paths.get(home, "python.exe").toString();
      } else
      {
        // On Linux, the binary is almost always in the 'bin' folder
        // relative to the HOME/Prefix.
        return Paths.get(home, "bin", "python3").toString();
      }
    }

    String suffix = isWindows ? ".exe" : "";
    String onPath = checkPath("python" + suffix);
    if (onPath != null)
      return onPath;

    throw new RuntimeException("Unable to locate Python executable");
  }

  private String makeHash(String path)
  {
    // No need to be cryptographic here.  We just need a unique key
    long hash = 0;
    for (int i = 0; i < path.length(); ++i)
    {
      hash = hash * 0x19185193123l + path.charAt(i);
    }
    return Long.toHexString(hash);
  }

  private void executeProbe(Properties probed)
  {
    try
    {
      // Load the script from resources instead of a constant
      String script = loadProbeResource();

      String[] cmd =
      {
        pythonExecutable, "-c", script
      };
      LOGGER.info("Executing Detective Probe from resources");

      ProcessBuilder pb = new ProcessBuilder(cmd);

      String jpypeDevPath = System.getProperty("jpype.path");
      if (jpypeDevPath != null)
      {
        String currentPath = pb.environment().getOrDefault("PYTHONPATH", "");
        String separator = isWindows ? ";" : ":";
        String newPath = jpypeDevPath + (currentPath.isEmpty() ? "" : separator + currentPath);
        pb.environment().put("PYTHONPATH", newPath);
        LOGGER.log(Level.INFO, "Probe PYTHONPATH augmented with: {0}", jpypeDevPath);
      }

      Process process = pb.start();

      try (InputStream is = process.getInputStream())
      {
        probed.load(is);
      }

      // Standard error handling using your existing logic
      try (BufferedReader err = new BufferedReader(new InputStreamReader(process.getErrorStream())))
      {
        String line;
        while ((line = err.readLine()) != null)
        {
          LOGGER.log(Level.WARNING, "Python Probe: {0}", line);
        }
      }

      if (process.waitFor() != 0)
        throw new RuntimeException("Python probe failed. Check logs.");
    } catch (IOException | InterruptedException ex)
    {
      throw new RuntimeException("Failed to execute detective probe", ex);
    }
  }

  /**
   * Resolves the OS-specific directory for JPype configuration and cache.
   *
   * * @return The Path to the JPype app data directory.
   */
  private Path getAppPath()
  {
    String homeDir = System.getProperty("user.home");
    // Windows uses AppData/Roaming, Unix uses a hidden dot-folder
    String appHome = isWindows ? "AppData/Roaming/JPype" : ".jpype";
    return Paths.get(homeDir, appHome);
  }

  /**
   * Store the results of the probe in the users home directory so we can skip
   * future probes.
   *
   * @param key is a hash code for this python environment.
   * @param exe is the location of the executable.
   */
  private void saveCache(String key, String exe, Properties probed)
  {
    LOGGER.log(Level.INFO, "Updating cache: {0} {1}", objs(key, exe));
    try
    {
      Path appPath = getAppPath(); // Helper to get .jpype or AppData path
      if (!Files.exists(appPath))
        Files.createDirectories(appPath);

      Path propFile = appPath.resolve(PROPERTIES);
      Properties cacheProps = new Properties();
      if (Files.exists(propFile))
      {
        try (InputStream is = Files.newInputStream(propFile))
        {
          cacheProps.load(is);
        }
      }

      // Store the exe location for reference
      cacheProps.setProperty(key, exe);

      // Store every property found by the probe with the unique hash prefix
      for (String name : probed.stringPropertyNames())
        cacheProps.setProperty(key + "-" + name, probed.getProperty(name));

      try (OutputStream os = Files.newOutputStream(propFile))
      {
        cacheProps.store(os, "JPype Environment Cache");
      }
    } catch (IOException ex)
    {
      LOGGER.log(Level.WARNING, "Could not save JPype cache: {0}", ex.getMessage());
    }
  }

  private boolean loadFromCache(String key, Properties probed)
  {
    Path propFile = getAppPath().resolve(PROPERTIES);
    LOGGER.log(Level.FINE, "Load from cache {0}", propFile);
    if (!Files.exists(propFile))
      return false;

    try (InputStream is = Files.newInputStream(propFile))
    {
      Properties allCache = new Properties();
      allCache.load(is);

      String prefix = key + "-";
      boolean found = false;
      for (String name : allCache.stringPropertyNames())
      {
        if (name.startsWith(prefix))
        {
          String cleanName = name.substring(prefix.length());
          probed.setProperty(cleanName, allCache.getProperty(name));
          LOGGER.log(Level.FINE, "  {0} {1}", objs(cleanName, allCache.getProperty(name)));
          found = true;
        }
      }

      // Final sanity check: does the cached library actually exist?
      if (found)
      {
        String lib = probed.getProperty(PYTHON_LIB);
        return lib != null && Files.exists(Paths.get(lib));
      }
    } catch (IOException ex)
    {
      LOGGER.log(Level.FINE, "Cache read failed: {0}", ex.getMessage());
    }
    return false;
  }

  private void resolveLibraries()
  {
    this.pythonExecutable = getExecutable();
    String key = makeHash(pythonExecutable);
    Properties probedProps = new Properties();

    boolean noCache = Boolean.parseBoolean(System.getProperty(JPYPE_NOCACHE, "false")); //
    boolean attemptInstall = Boolean.parseBoolean(System.getProperty(JPYPE_INSTALL, "false"));

    LOGGER.log(Level.FINE, "{0}={1}", objs(JPYPE_NOCACHE, noCache));
    LOGGER.log(Level.FINE, "{0}={1}", objs(JPYPE_INSTALL, attemptInstall));

    if (!noCache && loadFromCache(key, probedProps))
    {
      LOGGER.log(Level.INFO, "Cache hit: {0}", pythonExecutable);
    } else
    {
      try
      {
        executeProbe(probedProps);
      } catch (RuntimeException e)
      {
        if (attemptInstall)
        {
          LOGGER.info("JPype not found. Attempting binary installation via pip...");
          runPipInstall();
          executeProbe(probedProps); // Re-probe after install
        } else
        {
          throw e;
        }
      }
      saveCache(key, pythonExecutable, probedProps);
    }
    applyDefaults(probedProps);
    // Sync the instance variables with the now-probed System Properties
    this.pythonLibrary = System.getProperty(PYTHON_LIB);
    this.jpypeLibrary = System.getProperty(JPYPE_LIB);
    this.jpypeVersion = System.getProperty(JPYPE_VER);
  }

  /**
   * Attempts to install JPype1. Prioritizes local wheel files to support
   * offline environments.
   */
  private void runPipInstall()
  {
    try
    {
      // 1. Get requirements from the detective probe and context
      int[] v = parseVersion(NativeContext.VERSION);
      String versionReq = String.format("%d.%d", v[0], v[1]);
      String arch = System.getProperty(JPYPE_ARCH, "undetermined");
      LOGGER.log(Level.INFO, "Self-healer searching for local wheel matching: {0}", arch);

      // 2. Look for an explicit wheel in a local 'resources' or 'dep' folder
      Path localRepo = getAppPath().resolve("dep");
      Path explicitWheel = findLocalWheel(localRepo, versionReq, arch);

      String[] cmd;
      if (explicitWheel != null)
      {
        LOGGER.log(Level.INFO, "Found local wheel: {0}", explicitWheel.getFileName());
        cmd = new String[]
        {
          pythonExecutable, "-m", "pip", "install", explicitWheel.toString()
        };
      } else
      {
        // Fallback to network only if allowed
        LOGGER.log(Level.WARNING, "No local wheel found for {0}. Attempting network install...", arch);
        cmd = new String[]
        {
          pythonExecutable, "-m", "pip", "install",
          "JPype1>=" + versionReq, "--only-binary", ":all:"
        };
      }

      // 3. Execute
      LOGGER.log(java.util.logging.Level.INFO, "Running: {0}", String.join(" ", cmd));
      ProcessBuilder pb = new ProcessBuilder(cmd).inheritIO();
      if (pb.start().waitFor() != 0)
        throw new RuntimeException("Installation failed. Offline mode and no local wheel found for " + arch);
    } catch (Exception e)
    {
      throw new RuntimeException("Self-healer failed", e);
    }
  }

  /**
   * Scans a directory for a .whl file matching the version and architecture.
   */
  private Path findLocalWheel(Path dir, String version, String arch) throws IOException
  {
    if (!Files.exists(dir))
      return null;
    try (Stream<Path> stream = Files.list(dir))
    {
      return stream
              .filter(p -> p.toString().endsWith(".whl"))
              .filter(p -> p.toString().contains(version))
              .filter(p -> p.toString().contains(arch))
              .findFirst()
              .orElse(null);
    }
  }

  private void applyDefaults(Properties probed)
  {
    for (String name : probed.stringPropertyNames())
    {
      // override if not set externally
      if (System.getProperty(name) == null)
      {
        System.setProperty(name, probed.getProperty(name));
        LOGGER.log(Level.FINE, "Setting default: {0} = {1}", objs(name, probed.getProperty(name)));
      }
    }
  }

  private void installNatives()
  {
    // 3. Log the resolved environment details
    LOGGER.info("JPype Bridge Environment:");
    LOGGER.log(Level.INFO, "  Python Executable: {0}", this.pythonExecutable);
    LOGGER.log(Level.INFO, "  Python Library:    {0}", this.pythonLibrary);
    LOGGER.log(Level.INFO, "  JPype Library:     {0}", this.jpypeLibrary);
    LOGGER.log(Level.INFO, "  JPype Version:     {0}", this.jpypeVersion);

    try
    {
      if (!isWindows)
      {
        // We need the Python library loaded with global scope which is not possible from Java directly.
        String jpypeBootstrapLibrary = jpypeLibrary.replace("jpype.", "jpyne.");
        LOGGER.log(Level.FINE, "Loading Linux bootstrap: {0}", jpypeBootstrapLibrary);
        System.load(jpypeBootstrapLibrary);
        BootstrapLoader.loadLibrary(pythonLibrary);
      } else
      {
        System.load(pythonLibrary);
      }
      System.load(jpypeLibrary);
      LOGGER.info("Native libraries loaded successfully.");
    } catch (UnsatisfiedLinkError e)
    {
      LOGGER.log(Level.SEVERE, "Linkage Error: Check if architecture matches {0}", System.getProperty(JPYPE_ARCH));
      throw e;
    }
  }

  private boolean getBool(String key, String def)
  {
    return Boolean.parseBoolean(System.getProperty(key, def));
  }

  private Object[] objs(Object... obj)
  {
    return obj;
  }

//</editor-fold>
}
