package org.jpype.classloader;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.URLConnection;
import java.nio.file.FileSystems;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.nio.file.Paths;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

public class DynamicClassLoader extends URLClassLoader
{

  List<URLClassLoader> loaders = new LinkedList<>();
  HashMap<String, ArrayList<URL>> map = new HashMap<>();

  public DynamicClassLoader(ClassLoader parent)
  {
    super(launch(), parent);
  }

  /**
   * Special routine for handling non-ascii paths.
   *
   * If we are loaded as the system ClassLoader, then we will use
   * "jpype.class.path" rather than "java.class.path" during the load process.
   * We will move it into the expected place after so no one is the wiser.
   *
   * @return
   */
  private static URL[] launch()
  {
    String cp = System.getProperty("jpype.class.path");
    if (cp == null)
      return new URL[0];

    ArrayList<URL> path = new ArrayList<>();
    int last = 0;
    int next = 0;
    
    while (next!=-1)
    {
      // Find the parts
      next = cp.indexOf(File.pathSeparator, last);
      String element = (next == -1) ? cp.substring(last) : cp.substring(last, next);
      if (!element.isEmpty())
      {
        try
        {
          URL url = Paths.get(element).toUri().toURL();
          if (url != null)
            path.add(url);
        } catch (MalformedURLException ex)
        {
          System.err.println("Malformed url in classpath skipped " + element);
        }
      }
      last = next + 1;
    }

    // Replace the path
    System.clearProperty("jpype.class.path");
    System.setProperty("java.class.path", cp);
    return path.toArray(new URL[0]);
  }

  // this is required to add a Java agent even if it is already in the path
  @SuppressWarnings("unused")
  private void appendToClassPathForInstrumentation(String path) throws Throwable
  {
    addURL(Paths.get(path).toAbsolutePath().toUri().toURL());
  }

  public int getCode()
  {
    return loaders.hashCode();
  }

  /**
   * Add a set of jars to the classpath.
   *
   * @param root
   * @param glob
   * @throws IOException
   */
  public void addFiles(Path root, String glob) throws IOException
  {
    final PathMatcher pathMatcher = FileSystems.getDefault().getPathMatcher(glob);

    List<URL> urls = new LinkedList<>();
    Files.walkFileTree(root, new SimpleFileVisitor<Path>()
    {

      @Override
      public FileVisitResult visitFile(Path path,
              BasicFileAttributes attrs) throws IOException
      {
        if (pathMatcher.matches(root.relativize(path)))
        {
          URL url = path.toUri().toURL();
          urls.add(url);
        }
        return FileVisitResult.CONTINUE;
      }

      @Override
      public FileVisitResult visitFileFailed(Path file, IOException exc)
              throws IOException
      {
        return FileVisitResult.CONTINUE;
      }
    });

    loaders.add(new URLClassLoader(urls.toArray(new URL[0])));
  }

  public void addFile(Path path) throws FileNotFoundException
  {
    try
    {
      if (!Files.exists(path))
        throw new FileNotFoundException(path.toString());
      URL[] urls = new URL[]
      {
        path.toUri().toURL()
      };
      loaders.add(new URLClassLoader(urls));

      // Scan the file for directory entries
      this.scanJar(path);
    } catch (MalformedURLException ex)
    {
      // This should never happen
      throw new RuntimeException(ex);
    }
  }

  /**
   * Loads a class from the class loader.
   *
   * @param name is the name of the class with java class notation (using dots).
   * @return the class
   * @throws ClassNotFoundException was not found by the class loader.
   * @throws ClassFormatError if the class byte code was invalid.
   */
  @Override
  public Class findClass(String name) throws ClassNotFoundException, ClassFormatError
  {
    String aname = name.replace('.', '/') + ".class";
    URL url = this.getResource(aname);
    if (url == null)
      throw new ClassNotFoundException(name);

    try
    {
      URLConnection connection = url.openConnection();
      try (InputStream is = connection.getInputStream())
      {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        int bytes;
        byte[] d = new byte[1024];
        while ((bytes = is.read(d, 0, d.length)) != -1)
        {
          buffer.write(d, 0, bytes);
        }

        buffer.flush();
        byte[] data = buffer.toByteArray();
        return defineClass(name, data, 0, data.length);
      }
    } catch (IOException ex)
    {
    }
    throw new ClassNotFoundException(name);
  }

  @Override
  public URL getResource(String name)
  {
    // Search our parent
    URL url = this.getParent().getResource(name);
    if (url != null)
      return url;

    // Otherwise search locally
    return findResource(name);
  }

  @Override
  public URL findResource(String name)
  {
    // Check local first
    URL url = super.findResource(name);
    if (url != null)
      return url;

    // Use one of the subs
    for (URLClassLoader cl : this.loaders)
    {
      url = cl.findResource(name);
      if (url != null)
        return url;
    }
    // Both with and without / should generate the same result
    if (name.endsWith("/"))
      name = name.substring(0, name.length() - 1);
    if (map.containsKey(name))
      return map.get(name).get(0);
    return null;
  }

  @Override
  public Enumeration<URL> getResources(String name) throws IOException
  {
    ArrayList<URL> out = new ArrayList<>();
    out.addAll(Collections.list(getParent().getResources(name)));
    out.addAll(Collections.list(super.getResources(name)));
    for (URLClassLoader cl : this.loaders)
    {
      out.addAll(Collections.list(cl.findResources(name)));
    }
    // Both with and without / should generate the same result
    if (name.endsWith("/"))
      name = name.substring(0, name.length() - 1);
    if (map.containsKey(name))
      out.addAll(map.get(name));
    return Collections.enumeration(out);
  }

  public void addResource(String name, URL url)
  {
    if (!this.map.containsKey(name))
      this.map.put(name, new ArrayList<>());
    this.map.get(name).add(url);
  }

  /**
   * Recreate missing directory entries for Jars that lack indexing.
   *
   * Some jar files are missing the directory entries that prevents use from
   * properly importing their contents. This procedure scans a jar file when
   * loaded to build missing directories.
   *
   * @param p1
   */
  public void scanJar(Path p1)
  {
    if (!Files.exists(p1))
      return;
    if (Files.isDirectory(p1))
      return;
    try (JarFile jf = new JarFile(p1.toFile()))
    {
      Enumeration<JarEntry> entries = jf.entries();
      URI abs = p1.toAbsolutePath().toUri();
      Set urls = new java.util.HashSet();
      while (entries.hasMoreElements())
      {
        JarEntry next = entries.nextElement();
        String name = next.getName();

        // Skip over META-INF
        if (name.startsWith("META-INF/"))
          continue;

        if (next.isDirectory())
        {
          // If we find a directory entry then the jar has directories already
          return;
        }

        // Split on each separator in the name
        int i = 0;
        while (true)
        {
          i = name.indexOf("/", i);
          if (i == -1)
            break;
          String name2 = name.substring(0, i);

          i++;

          // Already have an entry no problem
          if (urls.contains(name2))
            continue;

          // Add a new entry for the missing directory
          String jar = "jar:" + abs + "!/" + name2 + "/";
          urls.add(name2);
          this.addResource(name2, new URL(jar));
        }
      }
    } catch (IOException ex)
    {
      // Anything goes wrong skip it
    }
  }

}
