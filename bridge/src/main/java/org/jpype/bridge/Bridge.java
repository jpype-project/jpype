/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 */
package org.jpype.bridge;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.Properties;

/**
 * This is a singleton the is created once to connect to Python.
 *
 * @author nelson85
 */
public class Bridge
{

    static Bridge instance = null;
    private String jpypeLibrary;
    private String pythonLibrary;
    private String jpypeVersion;
    private boolean isWindows = false;

    static final String WINDOWS_PROBE = ""
            + "import sysconfig\n"
            + "import os\n"
            + "gcv = sysconfig.get_config_var\n"
            + "print(os.path.join(gcv('BINDIR'), 'python'+gcv('VERSION')+'.dll'))\n"
            + "import _jpype\n"
            + "print(_jpype.__file__)\n"
            + "print(_jpype.__version__)\n";

    static final String UNIX_PROBE = ""
            + "import sysconfig\n"
            + "import os\n"
            + "gcv = sysconfig.get_config_var\n"
            + "print(os.path.join(gcv('LIBDIR'), gcv('LDLIBRARY')))\n"
            + "import _jpype\n"
            + "print(_jpype.__file__)\n"
            + "print(_jpype.__version__)\n";

    public static Bridge create()
    {
        if (instance == null)
        {
            Bridge bridge = new Bridge();
            bridge.launch();
            instance = bridge;
        }
        return instance;
    }

    private void launch()
    {
        // Tasks here
        //  1) Figure out what python we will use.
        //     using System.getProperty("python.home")
        //     using PYTHONHOME
        //     using command line
        //  2) Figure out where libpython is so we can launch a shell.
        //
        //  3) Figure out where jpype and its support library are in Python
        //   - Check the property file cache
        //   - Call Python executable and execute the search script.
        //     (save the result to the cache so we don't need to probe a second time)
        //  
        //  4) Import org.jpype and load the module to create entry points.
        //
        //  5) Call the entry point in org.jpype

        checkWindows();
        loadLibraries();
        instance = new Bridge();
    }

    public void loadLibraries()
    {
        // Get the _jpype extension library
        resolveLibraries();
        if (jpypeLibrary == null || pythonLibrary == null)
        {
            throw new RuntimeException("Unable to find _jpype module");
        }

        // Load libraries in Java so they are available for native calls.
        if (Paths.get(pythonLibrary).isAbsolute())
        {
            System.out.println("Load "+pythonLibrary);
            System.load(pythonLibrary);
        }
        else
        {
            System.out.println("Load2 "+pythonLibrary);
            System.loadLibrary(pythonLibrary);
        }

        // Our native points are in the Python native module
        System.out.println("Load "+jpypeLibrary);
        System.load(jpypeLibrary);

        // Add to FFI name lookup table
//    Native.addLibrary(pythonLibrary);
//    Native.addLibrary(jpypeLibrary);
//
//    // Start the Python
//    Native.start();
// 
//    // Connect up the natives
//    Statics.FRAME_STATIC = PyTypeManager.getInstance().createStaticInstance(PyFrameStatic.class);
//    return new EngineImpl();
    }

    /**
     * Determine if this is windows system, because everything is different on
     * windows.
     */
    public void checkWindows()
    {
        String osName = System.getProperty("os.name");
        if (osName.startsWith("Windows"))
            this.isWindows = true;
    }

    public String checkPath(String exec)
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

    public String getExecutable()
    {
        // Was is supplied via Java
        String out = System.getProperty("python.executable");

        // Was it passed as environment variable
        if (out != null)
            return out;

        String suffix = isWindows ? ".exe" : "";
        String home = System.getenv("PYTHONHOME");
        if (home != null)
            return String.format("%s%sbin%spython3%s", home, File.separator, File.separator, suffix);

        String onPath = checkPath("python3" + suffix);
        System.out.println(onPath);
        if (onPath != null)
            return onPath;

        throw new RuntimeException("Unable to locate Python executable");
    }

    public boolean checkCache(String key)
    {
        String homeDir = System.getProperty("user.home");
        String appHome = isWindows ? "\\AppData\\Roaming\\JPype" : ".jpype";
        Path appPath = Paths.get(homeDir, appHome);
        if (!Files.exists(appPath))
            return false;
        Properties prop = new Properties();
        try (InputStream is = Files.newInputStream(appPath))
        {
            String parameters;
            prop.load(is);
            parameters = (String) prop.get(key + "-python.lib");
            if (parameters == null)
                return false;
            this.pythonLibrary = parameters;
            parameters = (String) prop.get(key + "-jpype.lib");
            if (parameters == null)
                return false;
            this.jpypeLibrary = parameters;
            parameters = (String) prop.get(key + "-jpype.version");
            if (parameters == null)
                return false;
            this.jpypeVersion = parameters;
            return true;
        } catch (IOException ex)
        {
            return false;
        }
    }

    public void saveCache(String key, String exe)
    {
        try
        {
            System.out.println("Save cache");
            String homeDir = System.getProperty("user.home");
            String appHome = isWindows ? "\\AppData\\Roaming\\JPype" : ".jpype";
            Path appPath = Paths.get(homeDir, appHome);
            System.out.println(appPath);
            if (!Files.exists(appPath))
                Files.createDirectories(appPath);

            Path propFile = appPath.resolve("jpype.properties");
            Properties prop = new Properties();
            if (Files.exists(propFile))
            {
                System.out.println("Read " + propFile);
                try (InputStream is = Files.newInputStream(propFile))
                {
                    prop.load(is);
                }
            }
            prop.setProperty(key, exe);
            prop.setProperty(key + "-python.lib", this.pythonLibrary);
            prop.setProperty(key + "-jpype.lib", this.jpypeLibrary);
            prop.setProperty(key + "-jpype.version", this.jpypeVersion);

            System.out.println("Write " + propFile);
            try (OutputStream os = Files.newOutputStream(propFile))
            {
                prop.store(os, "");
            }

        } catch (IOException ex)
        {
            System.out.println(ex);
            // do nothing if we can't cache our variables
            return;
        }
    }

    public String makeHash(String path)
    {
        // No need to be cryptographic here.  We just need a unique key
        long hash = 0;
        for (int i = 0; i < path.length(); ++i)
        {
            hash = hash * 0x19185193123l + path.charAt(i);
        }
        return Long.toHexString(hash);
    }

    public void resolveLibraries()
    {
        // System properties dub compiled in paths
        this.pythonLibrary = System.getProperty("python.lib", pythonLibrary);

        // No need to do a probe
        if (this.jpypeLibrary != null && this.pythonLibrary != null)
            return;

        String pythonExecutable = getExecutable();
        String key = makeHash(pythonExecutable);
        if (checkCache(key))
            return;

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
            if (rc != 0)
                throw new RuntimeException("Python was unable to be probed.  Check stderr for details.");

            if (pythonLibrary == null)
                pythonLibrary = a;
            if (jpypeLibrary == null)
                jpypeLibrary = b;
            if (jpypeVersion == null)
                jpypeVersion = c;

            if (pythonLibrary == null || !Files.exists(Paths.get(pythonLibrary)))
                throw new RuntimeException("Unable to locate Python shared library");
            if (jpypeLibrary == null || !Files.exists(Paths.get(jpypeLibrary)))
                throw new RuntimeException("Unable to locate JPype shared library");
            if (jpypeVersion == null) // FIXME check version here
                throw new RuntimeException("Incorrect JPype version");

            saveCache(key, pythonExecutable);

            // FIXME we need to check to see if JPype is equal to or newer than
            // the wrapper version.  Else we will fail to operate properly.
        } catch (InterruptedException | IOException ex)
        {
            throw new RuntimeException("Failed to find JPype resources");
        }
    }

    public static void main(String[] args)
    {
        System.out.println("Hello World!");
        create();
        System.out.println(instance.isWindows);
        System.out.println(instance.jpypeLibrary);
        System.out.println(instance.jpypeVersion);
        System.out.println(instance.pythonLibrary);
    }
}
