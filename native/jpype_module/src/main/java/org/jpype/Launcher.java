// --- file: org/jpype/Launcher.java ---
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

import org.jpype.internal.NativeContext;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.File;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.stream.Stream;

/**
 * Locates, probes, and loads a Python environment.
 *
 * <p>
 * This is the "find and load a Python environment" half of what used to be
 * {@code MainInterpreter}: resolving which {@code python} executable to use,
 * running the detective probe (or loading its result from the on-disk
 * cache), self-healing via {@code pip} if the probe fails, and loading the
 * native {@code libpython}/{@code _jpype} libraries. It has no knowledge of
 * the {@code Interpreter} lifecycle or the CLI entry point -- those stay on
 * {@link MainInterpreter}, which owns one {@code Launcher} instance and
 * delegates environment resolution to it.</p>
 */
class Launcher
{

  final static Logger LOGGER = Logger.getLogger(Launcher.class.getName());
  final static String PROBE = "/org/jpype/resources/probe.py";

  final static String PYTHON_EXEC = "python.executable";
  final static String PYTHON_LIB = "python.lib";

  final static String JPYPE_LIB = "jpype.lib";
  final static String JPYPE_ARCH = "jpype.arch";
  final static String JPYPE_NOCACHE = "jpype.nocache";
  final static String JPYPE_INSTALL = "jpype.install";
  final static String JPYPE_VER = "jpype.version";

  final static String PROPERTIES = "jpype.properties";

  /**
   * Indicates whether the operating system is Windows.
   */
  private final boolean isWindows = checkWindows();

  private String pythonExecutable;

  /**
   * Path to the Python library.
   */
  private String pythonLibrary;

  /**
   * Path to the JPype library.
   */
  private String jpypeLibrary;

  /**
   * Version of the JPype library.
   */
  private String jpypeVersion;

  public String getPythonExecutable()
  {
    return pythonExecutable;
  }

  public String getPythonLibrary()
  {
    return pythonLibrary;
  }

  public String getJpypeLibrary()
  {
    return jpypeLibrary;
  }

  public String getJpypeVersion()
  {
    return jpypeVersion;
  }

  public boolean isPrepared()
  {
    return this.pythonLibrary != null;
  }

  /**
   * Parses a version string into an integer array.
   *
   * @param version The version string to parse (e.g., "3.9.7").
   * @return An integer array representing the version (e.g., [3, 9, 7]).
   */
  public static int[] parseVersion(String version)
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
   * Resolves the Python environment.
   *
   * <p>
   * Runs the detective probe (or loads from cache), identifies the correct
   * libraries, and performs self-healing if necessary. No-op if already
   * prepared. Does not load the native libraries -- see
   * {@link #installNatives()}, which the caller should invoke separately
   * once it has validated the resolved paths (mirroring
   * {@code MainInterpreter.start()}'s audit-then-load sequence).
   * </p>
   */
  public void prepare()
  {
    if (this.pythonLibrary != null)
      return; // Already prepared

    resolveLibraries();
  }

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
    // Get the module that contains the Launcher class
    Module module = Launcher.class.getModule();

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

      // Final sanity check: do the cached libraries actually exist? Checking
      // only python.lib was not enough -- jpype.lib can go stale on its own
      // (e.g. `pip install --upgrade JPype1` in the same venv moves the
      // native module to a new wheel-cache path) while python.lib still
      // exists, leaving a cache hit that later fails with a much less
      // informative UnsatisfiedLinkError out of installNatives().
      if (found)
      {
        String pythonLib = probed.getProperty(PYTHON_LIB);
        String jpypeLib = probed.getProperty(JPYPE_LIB);
        return pythonLib != null && Files.exists(Paths.get(pythonLib))
                && jpypeLib != null && Files.exists(Paths.get(jpypeLib));
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

  public void installNatives()
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

  private Object[] objs(Object... obj)
  {
    return obj;
  }

}
