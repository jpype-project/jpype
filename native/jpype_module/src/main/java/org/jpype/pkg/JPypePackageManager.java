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

import java.io.IOException;
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
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import org.jpype.JPypeContext;
import org.jpype.JPypeKeywords;

/**
 * Manager for the contents of a package.
 *
 * This class uses a number of tricks to provide a way to determine what
 * packages are available in the class loader. It searches the jar path, the
 * boot path (Java 8), and the module path (Java 9+). It does not currently work
 * with alternative classloaders. This class was rumored to be unobtainium as
 * endless posts indicated that it wasn't possible to determine the contents of
 * a package in general nor to retrieve the package contents, but this appears
 * to be largely incorrect as the jar and jrt file system provide all the
 * required methods.
 *
 */
public class JPypePackageManager
{

  final static List<FileSystem> bases = new ArrayList();
  final static List<ModuleDirectory> modules = getModules();
  final static FileSystemProvider jfsp = getFileSystemProvider("jar");
  final static Map<String, String> env = new HashMap<>();
  final static LinkedList<FileSystem> fs = new LinkedList<>();

  /**
   * Checks if a package exists.
   *
   * @param name is the name of the package.
   * @return true if this is a Java package either in a jar, module, or in the
   * boot path.
   */
  public static boolean isPackage(String name)
  {
    if (name.indexOf('.') != -1)
      name = name.replace(".", "/");
    if (isModulePackage(name) || isBasePackage(name) || isJarPackage(name))
      return true;
    return false;
  }

  /**
   * Get the list of the contents of a package.
   *
   * @param packageName
   * @return the list of all resources found.
   */
  public static Map<String, URI> getContentMap(String packageName)
  {
    Map<String, URI> out = new HashMap<>();
    packageName = packageName.replace(".", "/");
    // We need to merge all the file systems into one view like the classloader
    getJarContents(out, packageName);
    getBaseContents(out, packageName);
    getModuleContents(out, packageName);
    return out;
  }

  /**
   * Convert a URI into a path.
   *
   * This has special magic methods to deal with jar file systems.
   *
   * @param uri is the location of the resource.
   * @return the path to the uri resource.
   */
  static Path getPath(URI uri)
  {
    try
    {
      return Paths.get(uri);
    } catch (java.nio.file.FileSystemNotFoundException ex)
    {
    }

    if (uri.getScheme().equals("jar"))
    {
      try
      {
        // Limit the number of filesystems open at any one time
        fs.add(jfsp.newFileSystem(uri, env));
        if (fs.size() > 8)
          fs.removeFirst().close();
        return Paths.get(uri);
      } catch (IOException ex)
      {
      }
    }
    throw new FileSystemNotFoundException("Unknown filesystem for " + uri);
  }

  /**
   * Retrieve the Jar file system.
   *
   * @return
   */
  private static FileSystemProvider getFileSystemProvider(String str)
  {
    for (FileSystemProvider fsp : FileSystemProvider.installedProviders())
    {
      if (fsp.getScheme().equals(str))
        return fsp;
    }
    throw new FileSystemNotFoundException("Unable to find filesystem for " + str);
  }

//<editor-fold desc="java 8" defaultstate="collapsed">
  /**
   * Older versions of Java do not have a file system for boot packages. Thus
   * rather working through the classloader, we will instead probe java to get
   * the rt.jar. Crypto is a special case as it has its own jar. All other
   * resources are sourced through the regular jar loading method.
   */
  static
  {
    env.put("create", "true");

    ClassLoader cl = ClassLoader.getSystemClassLoader();
    URI uri = null;
    try
    {
      // This is for Java 8 and earlier in which the API jars are in rt.jar
      // and jce.jar
      uri = cl.getResource("java/lang/String.class").toURI();
      if (uri != null && uri.getScheme().equals("jar"))
      {
        FileSystem fs = jfsp.newFileSystem(uri, env);
        if (fs != null)
          bases.add(fs);
      }
      uri = cl.getResource("javax/crypto/Cipher.class").toURI();
      if (uri != null && uri.getScheme().equals("jar"))
      {
        FileSystem fs = jfsp.newFileSystem(uri, env);
        if (fs != null)
          bases.add(fs);
      }
    } catch (URISyntaxException | IOException ex)
    {
    }
  }

  private static void getBaseContents(Map<String, URI> out, String packageName)
  {
    for (FileSystem b : bases)
    {
      collectContents(out, b.getPath(packageName));
    }
  }

  /**
   * Check if a name is a package in the java bootstrap classloader.
   *
   * @param name
   * @return
   */
  private static boolean isBasePackage(String name)
  {
    try
    {
      if (name.isEmpty())
        return false;
      for (FileSystem jar : bases)
      {
        if (Files.isDirectory(jar.getPath(name)))
          return true;
      }
      return false;
    } catch (Exception ex)
    {
      throw new RuntimeException("Fail checking package '" + name + "'", ex);
    }
  }

//</editor-fold>
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
    ClassLoader cl = JPypeContext.getInstance().getClassLoader();
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
    ClassLoader cl = JPypeContext.getInstance().getClassLoader();
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
          out.put(JPypeKeywords.wrap(filename), toURI(file));
          continue;
        }
        // Skip inner classes
        if (filename.contains("$"))
          continue;

        // Include class files
        if (filename.endsWith(".class"))
        {
          String key = JPypeKeywords.wrap(filename.substring(0, filename.length() - 6));
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
      uri = new URI(
              uri.getScheme(),
              URLDecoder.decode(uri.getSchemeSpecificPart(), StandardCharsets.UTF_8),
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
