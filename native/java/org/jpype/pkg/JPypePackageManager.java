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
   * @return
   */
  public static Map<String, Path> getContentMap(String packageName)
  {
    Map<String, Path> out = new HashMap<>();
    packageName = packageName.replace(".", "/");
    getJarContents(out, packageName);
    getBaseContents(out, packageName);
    getModuleContents(out, packageName);
    return out;
  }

//<editor-fold desc="java 8" defaultstate="collapsed">
  static
  {
    URI uri = null;
    try
    {
      // This is for Java 8 and earlier in which the API jars are in rt.jar
      // and jce.jar
      uri = cl.getResource("java/lang/String.class").toURI();
      FileSystem fs = getJarFileSystem(uri);
      if (fs != null)
        bases.add(fs);
      uri = cl.getResource("javax/crypto/Cipher.class").toURI();
      fs = getJarFileSystem(uri);
      if (fs != null)
        bases.add(fs);
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

  private static boolean isBasePackage(String name)
  {
    try
    {
      for (FileSystem jar : bases)
      {
        if (Files.isDirectory(jar.getPath(name)))
          return true;
      }
      return false;
    } catch (Exception ex)
    {
      throw new RuntimeException("Fail while checking package " + name, ex);
    }
  }

//</editor-fold>
//<editor-fold desc="java 9" defaultstate="collapsed">
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
      return zipfs.getPath(q[1]);
    }
    throw new UnsupportedOperationException("Unknown filesystem for " + resource);
  }

//</editor-fold>
//<editor-fold desc="utility" defaultstate="collapsed">
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
        if (filename.contains("$"))
          continue;
        if (filename.endsWith(".class"))
        {
          String key = JPypeKeywords.wrap(filename.substring(0, filename.length() - 6));
          out.put(key, file);
        }
      }
    } catch (IOException ex)
    {
    }
  }
//</editor-fold>
}
