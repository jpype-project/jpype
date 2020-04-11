package org.jpype.pkg;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.ProviderNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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

  final static Map<URI, FileSystem> jars = new HashMap<>();
  final static List<FileSystem> bases = new ArrayList();
  final static ClassLoader cl = ClassLoader.getSystemClassLoader();
  final static List<ModuleDirectory> modules = getModules();

  /**
   * Checks if a package exists.
   *
   * @param name is the name of the package.
   * @return true if this is a Java package either in a jar, module, or in the
   * boot path.
   */
  public static boolean isPackage(String name)
  {
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
  public static Map<String, Path> getContentMap(String packageName)
  {
    Map<String, Path> out = new HashMap<>();
    packageName = packageName.replace(".", "/");
    // We need to merge all the file systems into one view like the classloader
    getJarContents(out, packageName);
    getBaseContents(out, packageName);
    getModuleContents(out, packageName);
    return out;
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
    URI uri = null;
    try
    {
      // This is for Java 8 and earlier in which the API jars are in rt.jar
      // and jce.jar
      uri = cl.getResource("java/lang/String.class").toURI();
      if (uri != null)
      {
        FileSystem fs = getJarFileSystem(uri);
        if (fs != null)
          bases.add(fs);
      }
      uri = cl.getResource("javax/crypto/Cipher.class").toURI();
      if (uri != null)
      {
        FileSystem fs = getJarFileSystem(uri);
        if (fs != null)
          bases.add(fs);
      }
    } catch (URISyntaxException ex)
    {
    }
  }

  private static void getBaseContents(Map<String, Path> out, String packageName)
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
  private static void getModuleContents(Map<String, Path> out, String name)
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
    try
    {
      Enumeration<URL> resources = cl.getResources(name);
      while (resources.hasMoreElements())
      {
        URI uri = resources.nextElement().toURI();
        if (Files.isDirectory(getJarPath(uri)))
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
  private static void getJarContents(Map<String, Path> out, String packageName)
  {
    try
    {
      String path = packageName.replace('.', '/');
      Enumeration<URL> resources = cl.getResources(path);
      while (resources.hasMoreElements())
      {
        URI resource = resources.nextElement().toURI();
        Path path2 = getJarPath(resource);
        collectContents(out, path2);
      }
    } catch (IOException | URISyntaxException ex)
    {
    }
  }

  /**
   * Get the "FileSystem" for the internals of a Jar.
   *
   * We will use a caching system so that we don't open the same jar multiple
   * times. It is not clear if this caching is necessary.
   *
   * @param uri is the uri with a "jar" scheme
   * @return the file system or null if not found.
   */
  private static FileSystem getJarFileSystem(URI uri)
  {
    if (!"jar".equals(uri.getScheme()))
      return null;
    if (jars.containsKey(uri))
      return jars.get(uri);
    Map<String, String> env = new HashMap<>();
    env.put("create", "true");
    FileSystem zipfs = jars.get(uri);
    if (zipfs == null)
    {
      try
      {
        zipfs = FileSystems.newFileSystem(uri, env);
        jars.put(uri, zipfs);
      } catch (IOException ex)
      {
      }
    }
    return zipfs;
  }

  /**
   * Convert a URI into a Path.
   *
   * Jar files are a special file system which extends into the jar structure.
   * We need to be able to handle both normal class files on a regular file
   * system and class files inside of jars.
   *
   * @param resource is the name of the resource as it appears from the
   * classloader.
   * @return is the path to the resource or null if not available.
   */
  private static Path getJarPath(URI resource)
  {
    if ("file".equals(resource.getScheme()))
    {
      return Paths.get(resource);
    }
    if ("jar".equals(resource.getScheme()))
    {
      String[] q = resource.toString().split("!");
      FileSystem zipfs = getJarFileSystem(URI.create(q[0]));
      if (zipfs != null)
        return zipfs.getPath(q[1]);
    }
    throw new UnsupportedOperationException("Unknown filesystem for " + resource);
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
  private static void collectContents(Map<String, Path> out, Path path2)
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
          out.put(JPypeKeywords.wrap(filename), file);
          continue;
        }
        // Skip inner classes
        if (filename.contains("$"))
          continue;

        // Include class files
        if (filename.endsWith(".class"))
        {
          String key = JPypeKeywords.wrap(filename.substring(0, filename.length() - 6));
          out.put(key, file);
        }

        // We can add other types of files here and import them in JPypePackage
        // as required.
      }
    } catch (IOException ex)
    {
    }
  }
//</editor-fold>
}
