package org.jpype.classloader;

import java.io.ByteArrayOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.net.URLConnection;
import java.nio.file.FileSystems;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;

public class DynamicClassLoader extends ClassLoader
{

  List<URLClassLoader> loaders = new LinkedList<>();

  public DynamicClassLoader(ClassLoader parent)
  {
    super(parent);
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

    loaders.add(new URLClassLoader(urls.toArray(new URL[urls.size()])));
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
      try ( InputStream is = connection.getInputStream())
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
    URL url = this.getParent().getResource(name);
    if (url != null)
      return url;
    for (ClassLoader cl : this.loaders)
    {
      url = cl.getResource(name);
      if (url != null)
        return url;
    }
    return null;
  }

  @Override
  public Enumeration<URL> getResources(String name) throws IOException
  {
    ArrayList<URL> out = new ArrayList<>();
    Enumeration<URL> urls = getParent().getResources(name);
    out.addAll(Collections.list(urls));
    for (URLClassLoader cl : this.loaders)
    {
      urls = cl.findResources(name);
      out.addAll(Collections.list(urls));
    }
    return Collections.enumeration(out);
  }
}
