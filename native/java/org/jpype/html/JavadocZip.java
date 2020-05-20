package org.jpype.html;

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.nio.file.DirectoryStream;
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

public class JavadocZip
{

  FileSystem fs;
  Path root;

  public JavadocZip(Path path) throws IOException
  {
    Map<String, String> env = new HashMap<>();
    env.put("create", "true");
    URI uri = URI.create("jar:" + path.toAbsolutePath().toUri().toString());
    fs = FileSystems.newFileSystem(uri, env, null);
    root = fs.getPath("docs");
  }

  /**
   * Get the documentation for the class if found.
   *
   * @param cls is the class to search for.
   * @return the html javadoc or null if not found.
   */
  public InputStream getInputStream(Class cls)
  {
    try
    {
      String[] parts = cls.getName().split("\\.");
      if (parts.length == 1)
        return null;
      parts[parts.length - 1] += ".html";
      Path path = fs.getPath("api", parts);
      return Files.newInputStream(root.resolve(path));
    } catch (IOException ex)
    {
      return null;
    }
  }

  public static void main(String[] args) throws IOException
  {
    JavadocZip zip = new JavadocZip(Paths.get("jdk-8u251-docs-all.zip"));
    System.out.println(zip.getInputStream(java.math.BigInteger.class));

  }
}
