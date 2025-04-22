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
package org.jpype.bridge;

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
import python.lang.PyObject;

/**
 * Frontend for the Python intepreter.
 *
 * This is a singleton the is created once to connect to Python. To start the
 * interpreter, set up all the configuration variables. Then call start.
 *
 *
 */
public class Interpreter
{

  static final String REQUIRED_VERSION = "1.6.0";
  static Interpreter instance = new Interpreter();
  private String jpypeLibrary;
  private String pythonLibrary;
  private String jpypeVersion;
  private final boolean isWindows = checkWindows();
  static Backend backend = null;
  public static PyObject stop = null;
  private boolean active = false;
  private final List<String> modulePaths = new ArrayList<>();

  // Probe pattern for windows which will retrieve the Python library location,
  // _jpype module location, and the version number.
  static final String WINDOWS_PROBE = ""
          + "import sysconfig\n"
          + "import os\n"
          + "gcv = sysconfig.get_config_var\n"
          + "print(os.path.join(gcv('BINDIR'), 'python'+gcv('VERSION')+'.dll'))\n"
          + "import _jpype\n"
          + "print(_jpype.__file__)\n"
          + "print(_jpype.__version__)\n";

  // Probe pattern for unix which will retrieve the Python library location,
  // _jpype module location, and the version number.
  static final String UNIX_PROBE = ""
          + "import sysconfig\n"
          + "import os\n"
          + "gcv = sysconfig.get_config_var\n"
          + "print(os.path.join(gcv('LIBDIR'), gcv('LDLIBRARY')))\n"
          + "import _jpype\n"
          + "print(_jpype.__file__)\n"
          + "print(_jpype.__version__)\n";

  private Interpreter()
  {
    String paths = System.getProperty("python.module.path");
    if (paths != null)
      this.modulePaths.addAll(Arrays.asList(paths.split(File.pathSeparator)));
  }

  public static Interpreter getInstance()
  {
    return instance;
  }

  /**
   * Use this list to add search paths for the Python modules.
   *
   * This can be used to limit the modules available for embedded applications.
   * If this list is not empty the default module path will not be used. Instead
   * this list will be the path.
   *
   * @return
   */
  public List<String> getModulePaths()
  {
    return modulePaths;
  }

  /**
   * Start the interpreter.Any configuration actions must have been completed
   * before the interpreter is started.
   *
   * Many configuration variables may be adjusted with Java System properties.
   *
   * @param args
   */
  public void start(String[] args)
  {
    // Once builtin is set internally then we can't call create again.
    if (Interpreter.backend != null)
      return;
    active = true;
    // Get the _jpype extension library
    resolveLibraries();

    // If we don't find the required libraries then we must fail.
    if (jpypeLibrary == null || pythonLibrary == null)
    {
      throw new RuntimeException("Unable to find _jpype module");
    }

    // Make sure the JPype we found is compatible
    int[] version = parseVersion(this.jpypeVersion);
    int[] required = parseVersion(REQUIRED_VERSION);
    if (version[0] < required[0]
            || (version[0] == required[0] && version[1] < required[1]))
      throw new RuntimeException("JPype version is too old.  Found " + this.jpypeLibrary);

    if (!isWindows)
    {
      // We need our preload hooks to get started.
      // On linux System.load() loads all symbols with RLTD_LOCAL which 
      // means they are not available for librarys to link against.  That
      // breaks the Python module loading system.  Thus on Linux or any
      // system that is similar we will need to load a bootstrap class which 
      // forces loading Python with global linkage prior to loading the 
      // first Python module.
      String jpypeBootstrapLibrary = jpypeLibrary.replace("jpype.c", "jpypeb.c");

      // First, load the preload hooks
      System.load(jpypeBootstrapLibrary);

      // Next, load libpython as a global library
      BootstrapLoader.loadLibrary(pythonLibrary);
    } else
    {
      // If no bootstrap is required we will simply preload the Python library.
      System.load(pythonLibrary);
    }

    // Finally, load the Python module
    System.load(jpypeLibrary);

    String[] paths = null;
    if (!this.modulePaths.isEmpty())
      paths = this.modulePaths.toArray(String[]::new);

    // There is a large pile of configuration variables to Python.
    //   I am not sure what will be important for different modes of operation.
    //   Best to pass most of them in from system properties.
    String program_name = System.getProperty("python.config.program_name");
    String prefix = System.getProperty("python.config.prefix");
    String home = System.getProperty("python.config.home");
    String exec_prefix = System.getProperty("python.config.exec_prefix");
    String executable = System.getProperty("python.config.executable");
    boolean isolated = Boolean.parseBoolean(System.getProperty("python.config.isolated", "false"));
    boolean fault_handler = Boolean.parseBoolean(System.getProperty("python.config.fault_handler", "false"));
    boolean quiet = Boolean.parseBoolean(System.getProperty("python.config.quiet", "false"));
    boolean verbose = Boolean.parseBoolean(System.getProperty("python.config.verbose", "false"));
    boolean site_import = Boolean.parseBoolean(System.getProperty("python.config.site_import ", "true"));
    boolean user_site = Boolean.parseBoolean(System.getProperty("python.config.user_site_directory ", "true"));
    boolean bytecode = Boolean.parseBoolean(System.getProperty("python.config.write_bytecode", "true"));

    // Start interpreter
    Natives.start(paths, args,
            program_name, prefix, home, exec_prefix, executable,
            isolated, fault_handler, quiet, verbose,
            site_import, user_site, bytecode);
  }

  public void interactive()
  {
    Natives.interactive();
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

//<editor-fold desc="internal" defaultstate="collapsed">
  /**
   * Determine if this is windows system, because everything is different on
   * windows.
   */
  private static boolean checkWindows()
  {
    String osName = System.getProperty("os.name");
    return osName.startsWith("Windows");
  }

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
   * Search the PATH for an executable.
   *
   * @param exec is the name of the executable.
   * @return the path found or null if not located.
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
   * Determine the location of the Python executable we will probe.
   *
   * This uses a series of methods.
   * <ul>
   * <li> (Application) Java system property "python.executable" </li>
   * <li> (User) Environment variable PYTHONHOME </li>
   * <li> (System) First python3 found in PATH </li>
   * </ul>
   *
   * @return the python executable location or null if not found.
   */
  private String getExecutable()
  {
    // Was is supplied via Java
    String out = System.getProperty("python.executable");

    // Was it passed as environment variable
    if (out != null)
      return out;

    String suffix = isWindows ? ".exe" : "";
    String home = System.getenv("PYTHONHOME");
    if (home != null)
      return Paths.get(home, "python" + suffix).toString();

    String onPath = checkPath("python" + suffix);
    if (onPath != null)
      return onPath;

    throw new RuntimeException("Unable to locate Python executable");
  }

  /**
   * Consult the cache to see if we have already probed this python
   * installation.
   *
   * @param key is a hash code associated with this Python install.
   * @return
   */
  private boolean checkCache(String key)
  {
    // Determine the location
    String homeDir = System.getProperty("user.home");
    String appHome = isWindows ? "\\AppData\\Roaming\\JPype" : ".jpype";
    Path appPath = Paths.get(homeDir, appHome);
    if (!Files.exists(appPath))
      return false;
    Properties properties = new Properties();

    // Load the properties
    Path propFile = appPath.resolve("jpype.properties");
    if (!Files.exists(propFile))
      return false;
    try (InputStream is = Files.newInputStream(propFile))
    {
      String parameters;
      properties.load(is);
      parameters = (String) properties.get(key + "-python.lib");
      if (parameters == null)
        return false;
      this.pythonLibrary = parameters;
      parameters = (String) properties.get(key + "-jpype.lib");
      if (parameters == null)
        return false;
      this.jpypeLibrary = parameters;
      parameters = (String) properties.get(key + "-jpype.version");
      if (parameters == null)
        return false;
      this.jpypeVersion = parameters;
      return true;
    } catch (IOException ex)
    {
      return false;
    }
  }

  /**
   * Store the results of the probe in the users home directory so we can skip
   * future probes.
   *
   * @param key is a hash code for this python environment.
   * @param exe is the location of the executable.
   */
  private void saveCache(String key, String exe)
  {
    try
    {
      // Determine where to store it
      String homeDir = System.getProperty("user.home");
      String appHome = isWindows ? "\\AppData\\Roaming\\JPype" : ".jpype";
      Path appPath = Paths.get(homeDir, appHome);

      // Create the path if it doesn't exist
      if (!Files.exists(appPath))
        Files.createDirectories(appPath);

      // Load the existing configuration
      Path propFile = appPath.resolve("jpype.properties");
      Properties prop = new Properties();
      if (Files.exists(propFile))
      {
        try (InputStream is = Files.newInputStream(propFile))
        {
          prop.load(is);
        }
      }

      // Store the executable name so the user knows which configuration this is for.
      prop.setProperty(key, exe);

      // Store the values we need
      prop.setProperty(key + "-python.lib", this.pythonLibrary);
      prop.setProperty(key + "-jpype.lib", this.jpypeLibrary);
      prop.setProperty(key + "-jpype.version", this.jpypeVersion);

      // Save back to disk
      try (OutputStream os = Files.newOutputStream(propFile))
      {
        prop.store(os, "");
      }

    } catch (IOException ex)
    {
      // do nothing if we can't cache our variables
    }
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

  private void resolveLibraries()
  {
    // System properties dub compiled in paths
    this.pythonLibrary = System.getProperty("python.lib", pythonLibrary);

    // No need to do a probe
    if (this.jpypeLibrary != null && this.pythonLibrary != null)
      return;

    // Find the Python executable
    String pythonExecutable = getExecutable();
    String key = makeHash(pythonExecutable);
    if (checkCache(key))
      return;

    // Probe the Python executeable for the values we need to start
    try
    {
      String[] cmd =
      {
        pythonExecutable, "-c", isWindows ? WINDOWS_PROBE : UNIX_PROBE
      };
      ProcessBuilder pb = new ProcessBuilder(cmd);
      pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
      Process process = pb.start();
      BufferedReader out = new BufferedReader(new InputStreamReader(process.getInputStream()));
      BufferedReader err = new BufferedReader(new InputStreamReader(process.getErrorStream()));
      int rc = process.waitFor();
      String a = out.readLine();
      String b = out.readLine();
      String c = out.readLine();

      // Dump stderr out to so users can see problems.
      String e = err.readLine();
      while (e != null)
      {
        System.err.println(e);
        e = err.readLine();
      }
      out.close();
      err.close();

      // Failed to run Python
      if (rc != 0)
        throw new RuntimeException(String.format("Python was unable to be probed.  Check stderr for details. (%d)", rc));

      // Copy over the values from stdout.
      if (pythonLibrary == null)
        pythonLibrary = a;
      if (jpypeLibrary == null)
        jpypeLibrary = b;
      if (jpypeVersion == null)
        jpypeVersion = c;

      // Verify that everything we need was found
      if (pythonLibrary == null || !Files.exists(Paths.get(pythonLibrary)))
        throw new RuntimeException("Unable to locate Python shared library");
      if (jpypeLibrary == null || !Files.exists(Paths.get(jpypeLibrary)))
        throw new RuntimeException("Unable to locate JPype shared library");
      if (jpypeVersion == null) // FIXME check version here
        throw new RuntimeException("Incorrect JPype version");

      // Update the cache
      saveCache(key, pythonExecutable);

      // FIXME we need to check to see if JPype is equal to or newer than
      // the wrapper version.  Else we will fail to operate properly.
    } catch (InterruptedException | IOException ex)
    {
      throw new RuntimeException("Failed to find JPype resources");
    }
  }

  public static void setBackend(Backend entry)
  {
    // This is the first entry point called from Python.
    // it should lock out calling the create method
    backend = entry;
    stop = backend.object();
  }

  public static Backend getBackend()
  {
    return backend;
  }
//</editor-fold>

  public static void main(String[] args)
  {
    // Start the interpreter
    Interpreter interpreter = getInstance();
    interpreter.start(args);
    interpreter.interactive();
    System.out.println("done");
  }
}
