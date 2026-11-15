// --- file: org/jpype/pkg/PackageManager.java ---
/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  See NOTICE file for details.
**************************************************************************** */
package org.jpype.pkg;

import java.io.BufferedReader;
import java.io.IOException;
import java.lang.module.ModuleReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystem;
import java.nio.file.FileSystemNotFoundException;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.ProviderNotFoundException;
import java.nio.file.spi.FileSystemProvider;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import org.jpype.internal.Keywords;

/**
 * Manages the contents of Java packages for JPype.
 *
 * <p>
 * This class provides functionality to determine the availability of packages
 * in the class loader and retrieve their contents. It searches through the jar
 * path, boot path (Java 8), and module path (Java 9+). The class does not
 * currently support alternative class loaders.</p>
 *
 * <p>
 * Although it was previously believed that determining package contents was
 * impossible, this class leverages the jar and jrt file systems to provide the
 * required methods for package inspection.</p>
 */
public class PackageManager
{
  static ClassLoader classloader = ClassLoader.getSystemClassLoader();

  final static List<FileSystem> bases = new ArrayList<>();
  final static List<ModuleDirectory> modules = getModules();
  final static FileSystemProvider jfsp = findFileSystemProvider("jar");
  final static Map<String, String> env = new HashMap<>();
  final static LinkedList<FileSystem> fs = new LinkedList<>();

  private PackageManager()
  {}
  
  public static void setClassLoader(ClassLoader cl)
  {
    classloader = cl;
  }
  
  /**
   * Checks if a package exists.
   *
   * <p>
   * Determines whether the specified package is available in a jar file,
   * module, or boot path.</p>
   *
   * @param name The name of the package.
   * @return {@code true} if the package exists; {@code false} otherwise.
   */
  public static boolean isPackage(String name)
  {
    String path = name.indexOf('.') != -1 ? name.replace(".", "/") : name;
    if (isModulePackage(path)
            || isNamedModulePackage(path.replace('/', '.'))
            || isJarPackage(path))
      return true;
    if (jfsp == null && modules.isEmpty())
      return isPackageAsset(path);
    return false;
  }

  /**
   * Asset name of the flat package-name list written by
   * project/android/recipes/jpype1/__init__.py's generate_package_markers
   * and bundled via project/android/testapp/buildozer.spec's
   * android.add_assets. One dotted package name per line.
   */
  private static final String ANDROID_PKG_LIST_ASSET = "jpype-android-packages.txt";

  /**
   * Cached Android AssetManager, resolved reflectively on first use (see
   * getAndroidAssetManager()). Not eagerly initialized: it depends on
   * org.kivy.android.PythonActivity, a class that plainly doesn't exist
   * on desktop, and even on Android may not have set its static
   * mActivity field yet at JPypePackageManager class-init time.
   */
  private static Object androidAssetManager;
  private static boolean androidAssetManagerResolved = false;

  /**
   * Lazily-loaded set of every package name from ANDROID_PKG_LIST_ASSET,
   * in dotted form. Null until the first isPackageAsset() call that has a
   * usable AssetManager; stays null forever on desktop or if the asset
   * can't be read, in which case isPackageAsset() always returns false.
   */
  private static Set<String> androidPackageNames;
  private static boolean androidPackageNamesLoaded = false;

  /**
   * Package check for platforms with no filesystem-based package
   * enumeration at all - e.g. Android's ART, which has neither a "jar"
   * FileSystemProvider nor a jrt:/ module filesystem (see
   * getFileSystemProvider() and getModules() above), so the checks above
   * can never do better than report "not found" for every name, including
   * perfectly valid ones like "java" or "java.lang". That broke
   * jpype.imports and jpype.JPackage(...) entirely on Android: the very
   * first, package-only step of resolving e.g. java.lang.String always
   * failed before ever reaching a class.
   * <p>
   * This mirrors what the desktop checks above do - "does this directory
   * exist in the classpath" - using an Android-appropriate substitute for
   * "directory": a flat list of every package the Android build recipe
   * found at build time (scanning android.jar plus this project's own
   * compiled/harness classes - see project/android/recipes/jpype1/
   * __init__.py's generate_package_markers), bundled as a single real
   * Android asset and read back via AssetManager (reached reflectively
   * through org.kivy.android.PythonActivity.mActivity - see
   * getAndroidAssetManager() below).
   * <p>
   * Two earlier, different-shaped versions of this (one marker file per
   * package, first under the Java source tree, then under the bootstrap's
   * own src/main/assets/) both failed to actually reach the built APK for
   * reasons that had nothing to do with AssetManager itself - see
   * generate_package_markers' docstring for the full account (Android
   * Gradle's java source set drops non-.java files; and separately,
   * bootstraps/common/build/build.py's make_package() wipes and
   * regenerates src/main/assets/ from scratch on every build, from only
   * webview_includes/ and explicit --asset args, discarding anything
   * written into it any other way). This version goes through that same
   * --asset mechanism, so it doesn't fight that regeneration step.
   *
   * @param path is the name to check, in path form (dots already
   * replaced with slashes by the caller).
   * @return true if path names a package found in the bundled list.
   */
  private static boolean isPackageAsset(String path)
  {
    Set<String> names = getAndroidPackageNames();
    if (names == null)
      return false;
    return names.contains(path.replace('/', '.'));
  }

  /**
   * Lazily read and cache ANDROID_PKG_LIST_ASSET into androidPackageNames.
   *
   * @return the set of known package names, or null if unavailable
   * (desktop, Android before startup finishes, or the asset is missing).
   */
  private static synchronized Set<String> getAndroidPackageNames()
  {
    if (androidPackageNamesLoaded)
      return androidPackageNames;
    Object assetManager = getAndroidAssetManager();
    if (assetManager == null)
      // Not necessarily permanent - e.g. Android before mActivity is set
      // yet - so don't mark loaded, a later call may still succeed.
      return null;
    try
    {
      Method open = assetManager.getClass().getMethod("open", String.class);
      InputStream is = (InputStream) open.invoke(assetManager, ANDROID_PKG_LIST_ASSET);
      Set<String> names = new HashSet<>();
      try (BufferedReader reader = new BufferedReader(
              new InputStreamReader(is, StandardCharsets.UTF_8)))
      {
        String line;
        while ((line = reader.readLine()) != null)
        {
          line = line.trim();
          if (!line.isEmpty())
            names.add(line);
        }
      }
      androidPackageNames = names;
    } catch (ReflectiveOperationException | IOException | ClassCastException ex)
    {
      // NoSuchMethodException/IllegalAccessException: reflection itself
      // failed. InvocationTargetException wraps AssetManager.open()'s
      // IOException when the asset is missing. Either way, this is a
      // permanent "no list available" - the app package doesn't get
      // rebuilt at runtime, so a later retry can't succeed either.
      androidPackageNames = null;
    }
    androidPackageNamesLoaded = true;
    return androidPackageNames;
  }

  /**
   * Reflectively resolve Android's real AssetManager for the running app,
   * through org.kivy.android.PythonActivity.mActivity (a public static
   * field p4a's own webview bootstrap sets to the running Activity - see
   * doc comment on isPackageAsset() for why this is needed instead of a
   * classloader resource lookup). Purely reflective so this file - shared
   * with the desktop build, where org.kivy.android.* doesn't exist at
   * all - still compiles fine there; a desktop run simply never finds
   * the class and caches a permanent null.
   *
   * @return the AssetManager instance, or null if unavailable (desktop,
   * or Android before PythonActivity has set mActivity).
   */
  private static synchronized Object getAndroidAssetManager()
  {
    if (androidAssetManagerResolved)
      return androidAssetManager;
    Class<?> activityClass;
    try
    {
      activityClass = Class.forName("org.kivy.android.PythonActivity");
    } catch (ClassNotFoundException ex)
    {
      // Definitely not this Android bootstrap (or not Android at all,
      // e.g. the desktop build) - this can never change, safe to cache.
      androidAssetManagerResolved = true;
      return null;
    }
    try
    {
      Object activity = activityClass.getField("mActivity").get(null);
      if (activity == null)
        // The class exists (we ARE on Android) but hasn't finished
        // starting up yet - don't cache, a later call may succeed.
        return null;
      Method getAssets = activity.getClass().getMethod("getAssets");
      androidAssetManager = getAssets.invoke(activity);
      androidAssetManagerResolved = true;
    } catch (ReflectiveOperationException ex)
    {
      androidAssetManagerResolved = true;
      androidAssetManager = null;
    }
    return androidAssetManager;
  }

  /**
   * Open a named Android asset (bundled via buildozer.spec's
   * android.add_assets - see this class's own ANDROID_PKG_LIST_ASSET use
   * for the pattern). Exposed publicly so other org.jpype classes that
   * need a static resource file which doesn't survive as a plain
   * classloader resource on Android (e.g. org.jpype.html.Html's
   * entities.txt - see that class) don't need their own copy of the
   * getAndroidAssetManager() reflection glue.
   *
   * @param name is the asset's path, as given on the right of the colon
   * in its android.add_assets entry.
   * @return an InputStream for the asset, or null if unavailable
   * (desktop, Android before startup finishes, or no such asset).
   */
  public static InputStream openAndroidAsset(String name)
  {
    Object assetManager = getAndroidAssetManager();
    if (assetManager == null)
      return null;
    try
    {
      Method open = assetManager.getClass().getMethod("open", String.class);
      return (InputStream) open.invoke(assetManager, name);
    } catch (ReflectiveOperationException | ClassCastException ex)
    {
      // NoSuchMethodException/IllegalAccessException: reflection itself
      // failed. InvocationTargetException wraps AssetManager.open()'s
      // IOException when the asset doesn't exist.
      return null;
    }
  }

  // A reflective Class.forName() probe was tried here as a fallback for
  // names with no marker asset ("can't prove it ISN'T a package, so
  // assume it is") - reverted. jpype.imports' meta_path finder
  // (jpype/imports.py's _JImportLoader.find_spec) decides whether to
  // claim a plain `import name` statement based solely on isPackage(name)
  // - "can't disprove it" is exactly the wrong default there: a genuinely
  // missing top-level package (e.g. `import numpy` when numpy isn't
  // installed) has no marker and isn't a loadable class either, so the
  // probe would return true and the finder would silently claim it,
  // producing a fake Java package object instead of letting Python's
  // normal import machinery raise ModuleNotFoundError - breaking the
  // common `try: import numpy \n except ImportError: pass` pattern
  // used throughout test/jpypetest. The marker-asset scan (see
  // isPackageAsset() above) is comprehensive for everything actually
  // reachable on Android's classpath - platform API plus whatever's
  // bundled into the dex - unlike on desktop, Android has no
  // addClassPath()-style mechanism to add packages the scan couldn't
  // have already seen (see doc/android.rst), so there is no real gap
  // for a fallback to cover.

  /**
   * Retrieves the contents of a package.
   *
   * <p>
   * Searches the jar path, base path, and module path to collect all resources
   * within the specified package.</p>
   *
   * @param packageName The name of the package.
   * @return A map containing the names of resources and their corresponding
   * URIs.
   */
  public static Map<String, URI> getContentMap(String packageName)
  {
    Map<String, URI> out = new HashMap<>();
    String dottedName = packageName.replace('/', '.');
    packageName = packageName.replace(".", "/");
    // We need to merge all the file systems into one view like the classloader
    getJarContents(out, packageName);
//    getBaseContents(out, packageName);
    getModuleContents(out, packageName);
    getNamedModuleContents(out, dottedName);
    return out;
  }

  /**
   * Converts a URI into a {@link Path}.
   *
   * <p>
   * This method handles special cases for jar file systems to ensure proper
   * resolution of paths.</p>
   *
   * @param uri The URI of the resource.
   * @return The {@link Path} corresponding to the URI.
   * @throws FileSystemNotFoundException If the file system for the URI cannot
   * be found.
   */
  static Path getPath(URI uri)
  {
    try
    {
      return Paths.get(uri);
    } catch (java.nio.file.FileSystemNotFoundException ex)
    {
      // Handle jar file system
    }

    if (uri.getScheme().equals("jar"))
    {
      try
      {
        // Limit the number of filesystems open at any one time
        fs.add(jfsp.newFileSystem(uri, env));
        if (fs.size() > 8)
        {
          fs.removeFirst().close();
        }
        return Paths.get(uri);
      } catch (IOException ex)
      {
        // Handle IO exceptions
      }
    }
    throw new FileSystemNotFoundException("Unknown filesystem for " + uri);
  }

  /**
   * Retrieves the file system provider for the specified scheme.
   *
   * @param str The scheme name (e.g., "jar").
   * @return The {@link FileSystemProvider} for the specified scheme.
   * @throws FileSystemNotFoundException If no provider is found for the scheme.
   */
  private static FileSystemProvider getFileSystemProvider(String str)
  {
    FileSystemProvider fsp = findFileSystemProvider(str);
    if (fsp == null)
      throw new FileSystemNotFoundException("Unable to find filesystem for " + str);
    return fsp;
  }

  /**
   * Like {@link #getFileSystemProvider(String)} but returns null, rather than
   * throwing, if no provider for this scheme is installed.  Android's ART has
   * no "jar" provider at all (there is no jar-based classpath there, see
   * doc/android.rst), and an exception here would fail this class's static
   * initializer, permanently poisoning every later use of the class.  Every
   * use of the provider is gated on a URI's scheme matching first, so a null
   * simply makes those branches unreachable.
   */
  private static FileSystemProvider findFileSystemProvider(String str)
  {
    for (FileSystemProvider fsp : FileSystemProvider.installedProviders())
    {
      if (fsp.getScheme().equals(str))
        return fsp;
    }
    return null;
  }

//<editor-fold desc="java 9" defaultstate="collapsed">
  /**
   * Get a list of all modules.
   *
   * This may be many modules or just a few. Limited distributes created using
   * jlink will only have a portion of the usual modules.
   *
   * @return
   */
  static List<ModuleDirectory> getModules()
  {
    ArrayList<ModuleDirectory> out = new ArrayList<>();
    try
    {
      FileSystem fs = FileSystems.getFileSystem(URI.create("jrt:/"));
      Path modulePath = fs.getPath("modules");
      for (Path module : Files.newDirectoryStream(modulePath))
      {
        out.add(new ModuleDirectory(module));
      }
    } catch (ProviderNotFoundException | IOException ex)
    {
    }
    return out;
  }

  /**
   * Check if a name corresponds to a package in a module.
   *
   * @param name
   * @return true if it is a package.
   */
  private static boolean isModulePackage(String name)
  {
    if (modules.isEmpty())
      return false;
    String[] split = name.split("/");
    String search = name;
    if (split.length > 3)
      search = String.join("/", Arrays.copyOfRange(split, 0, 3));
    for (ModuleDirectory module : modules)
    {
      if (module.contains(search))
      {
        if (Files.isDirectory(module.modulePath.resolve(name)))
          return true;
      }
    }
    return false;
  }

  /**
   * Retrieve the contents of a module by package name.
   *
   * @param out
   * @param name
   */
  private static void getModuleContents(Map<String, URI> out, String name)
  {
    if (modules.isEmpty())
      return;
    String[] split = name.split("/");
    String search = name;
    if (split.length > 3)
      search = String.join("/", Arrays.copyOfRange(split, 0, 3));
    for (ModuleDirectory module : modules)
    {
      if (module.contains(search))
      {
        Path path2 = module.modulePath.resolve(name);
        if (Files.isDirectory(path2))
          collectContents(out, path2);
      }
    }
  }

  /**
   * Modules are stored in the jrt filesystem.
   *
   * However, that is not a simple flat filesystem by path as the jrt files are
   * structured by package name. Thus we will need a separate structure which is
   * rooted at the top of each module.
   */
  private static class ModuleDirectory
  {

    List<String> contents = new ArrayList<>();
    private final Path modulePath;

    ModuleDirectory(Path module)
    {
      this.modulePath = module;
      listPackages(contents, module, module, 0);
    }

    boolean contains(String path)
    {
      for (String s : contents)
      {
        if (s.equals(path))
          return true;
      }
      return false;
    }

    private static void listPackages(List<String> o, Path base, Path p, int depth)
    {
      try
      {
        if (depth >= 3)
          return;
        for (Path d : Files.newDirectoryStream(p))
        {
          if (Files.isDirectory(d))
          {
            o.add(base.relativize(d).toString());
            listPackages(o, base, d, depth + 1);
          }
        }
      } catch (IOException ex)
      {
      }
    }
  }

//</editor-fold>
//<editor-fold desc="named application modules" defaultstate="collapsed">
  /**
   * Checks if a package belongs to a named (application) module resolved
   * into the boot layer - i.e. a module supplied via {@code --module-path}/
   * {@code --add-modules} on the launch command line, as opposed to a JDK
   * platform module (handled by {@link #isModulePackage}) or classpath/
   * unnamed-module content (handled by {@link #isJarPackage}).
   *
   * <p>
   * {@code ClassLoader.getResources(String)} (used by {@link #getJarContents})
   * does not reliably enumerate a named module's own package contents - only
   * classpath/unnamed-module resources and anything patched in via
   * {@code --patch-module}. Without this, a package that lives solely inside
   * the {@code org.jpype} module itself (e.g. {@code python.exceptions}, when
   * this code is embedded into a JVM launched with
   * {@code --module-path=.../org.jpype-*.jar --add-modules=org.jpype}) is
   * invisible to {@link #getContentMap} even though every class in it is
   * perfectly loadable by name.</p>
   *
   * @param dottedName The name of the package, dot-separated.
   * @return {@code true} if a boot-layer named module declares this package.
   */
  private static boolean isNamedModulePackage(String dottedName)
  {
    return findModuleForPackage(dottedName).isPresent();
  }

  private static Optional<java.lang.module.ResolvedModule> findModuleForPackage(String dottedName)
  {
    return ModuleLayer.boot().configuration().modules().stream()
            .filter(rm -> rm.reference().descriptor().packages().contains(dottedName))
            .findFirst();
  }

  /**
   * Retrieves the contents of a package declared by a named application
   * module, via {@link ModuleReader#list()} - the module-path counterpart to
   * {@link #getJarContents}. See {@link #isNamedModulePackage} for why this
   * is needed in addition to the classloader/jar-based lookup.
   *
   * @param out is the map to store the result in.
   * @param dottedName is the name of the package, dot-separated.
   */
  private static void getNamedModuleContents(Map<String, URI> out, String dottedName)
  {
    Optional<java.lang.module.ResolvedModule> found = findModuleForPackage(dottedName);
    if (!found.isPresent())
      return;
    String prefix = dottedName.replace('.', '/') + "/";
    try (ModuleReader reader = found.get().reference().open())
    {
      reader.list().forEach(resourceName ->
      {
        if (!resourceName.startsWith(prefix))
          return;
        String rest = resourceName.substring(prefix.length());
        // Only direct children (skip sub-packages and inner classes).
        if (rest.isEmpty() || rest.indexOf('/') != -1 || rest.contains("$"))
          return;
        if (!rest.endsWith(".class"))
          return;
        String key = Keywords.wrap(rest.substring(0, rest.length() - 6));
        try
        {
          reader.find(resourceName).ifPresent(uri -> out.put(key, uri));
        } catch (IOException ex)
        {
        }
      });
    } catch (IOException ex)
    {
    }
  }

//</editor-fold>
//<editor-fold desc="jar" defaultstate="collapsed">
  /**
   * Checks if a name corresponds to package in a jar file or on the classpath
   * filesystem.
   *
   * Classloaders provide a method to get all resources with a given name. This
   * is needed because the same package name may appear in multiple jars or
   * filesystems. We do not need to disambiguate it here, but just get a listing
   * that we can use to find a resource later.
   *
   * @param name is the name of the package to search for.
   * @return true if the name corresponds to a Java package.
   */
  private static boolean isJarPackage(String name)
  {
    ClassLoader cl = classloader;
    try
    {
      Enumeration<URL> resources = cl.getResources(name);
      while (resources.hasMoreElements())
      {
        URI uri = resources.nextElement().toURI();
        if (Files.isDirectory(getPath(uri)))
          return true;
      }
    } catch (IOException | URISyntaxException ex)
    {
    }
    return false;
  }

  /**
   * Retrieve a list of packages and classes stored on a file system or in a
   * jar.
   *
   * @param out is the map to store the result in.
   * @param packageName is the name of the package
   */
  private static void getJarContents(Map<String, URI> out, String packageName)
  {
    ClassLoader cl = classloader;
    try
    {
      String path = packageName.replace('.', '/');
      Enumeration<URL> resources = cl.getResources(path);
      while (resources.hasMoreElements())
      {
        URI resource = resources.nextElement().toURI();

        // Handle MRJAR format
        //   MRJAR may not report every directory but instead just the overlay.
        //   So we need to find the original and interogate it first before
        //   checking the version specific part.  Worst case we collect the
        //   contents twice.
        String schemePart = resource.getSchemeSpecificPart();
        int index = schemePart.indexOf("!");
        if (index != -1)
        {
          if (schemePart.substring(index + 1).startsWith("/META-INF/versions/"))
          {
            int index2 = schemePart.indexOf('/', index + 20);
            schemePart = schemePart.substring(0, index + 1) + schemePart.substring(index2);
            URI resource2 = new URI(resource.getScheme() + ":" + schemePart);
            Path path3 = getPath(resource2);
            collectContents(out, path3);
          }
        }

        Path path2 = getPath(resource);
        collectContents(out, path2);
      }
    } catch (IOException | URISyntaxException ex)
    {
    }
  }

//</editor-fold>
//<editor-fold desc="utility" defaultstate="collapsed">
  /**
   * Collect the contents from a path.
   *
   * This operates on jars, modules, and filesystems to collect the names of all
   * resources found. We skip over inner classes as those are accessed under
   * their included classes. For now we are not screening against other private
   * symbols.
   *
   * @param out is the map to store the result in.
   * @param path2 is a path holding a directory to probe.
   */
  private static void collectContents(Map<String, URI> out, Path path2)
  {
    try
    {
      for (Path file : Files.newDirectoryStream(path2))
      {
        String filename = file.getFileName().toString();
        if (Files.isDirectory(file))
        {
          // Same implementations add the path separator to the end of toString().
          if (filename.endsWith(file.getFileSystem().getSeparator()))
            filename = filename.substring(0, filename.length() - 1);
          out.put(Keywords.wrap(filename), toURI(file));
          continue;
        }
        // Skip inner classes
        if (filename.contains("$"))
          continue;

        // Include class files
        if (filename.endsWith(".class"))
        {
          String key = Keywords.wrap(filename.substring(0, filename.length() - 6));
          out.put(key, toURI(file));
        }

        // We can add other types of files here and import them in JPypePackage
        // as required.
      }
    } catch (IOException ex)
    {
    }
  }

  private static URI toURI(Path path)
  {
    URI uri = path.toUri();

    try
    {
      // Java 8 bug https://bugs.java.com/bugdatabase/view_bug.do?bug_id=8131067
      // Zip file system provider returns doubly % encoded URIs. We resolve this
      // by re-encoding the URI after decoding it.
      //
      // Replace "+" with its URL-encoded representation to avoid conversion to
      // a space and ensure correct round-trip encoding.
      uri = new URI(
              uri.getScheme(),
              // "+" is escaped before decoding so a literal "+" in a path
              // survives URLDecoder.decode (which would otherwise turn it
              // into a space) -- see #1413.
              URLDecoder.decode(uri.getSchemeSpecificPart().replace("+", "%2b"), StandardCharsets.UTF_8.name()),
              uri.getFragment()
      );

      // `toASCIIString` ensures the URI is URL encoded with only ascii
      // characters. This avoids issues in `sun.nio.fs.UnixUriUtils.fromUri` that
      // naively uses `uri.getRawPath()` despite the possibility that it contains
      // non-ascii characters that will cause errors. By using `toASCIIString` and
      // re-wrapping it in a URI object we ensure that the URI is properly
      // encoded. See: https://github.com/jpype-project/jpype/issues/1194
      return new URI(uri.toASCIIString());
    } catch (Exception e)
    {
      // This exception *should* never occur as we are re-encoding a valid URI.
      // Throwing a runtime exception avoids java exception handling boilerplate
      // for a situation that *should* never occur.
      throw new RuntimeException("Failed to encode URI: " + uri, e);
    }
  }
//</editor-fold>
}
