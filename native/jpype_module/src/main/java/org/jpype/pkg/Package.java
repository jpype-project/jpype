// --- file: org/jpype/pkg/Package.java ---
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

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Modifier;
import java.net.URI;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Map;
import org.jpype.internal.Keywords;

/**
 * Represents a Java package in JPype.
 *
 * <p>
 * This class provides the directory structure and attributes for a Java
 * package, enabling JPype imports. Most of the heavy lifting is done by the
 * {@link PackageManager}, which acts as a class loader to determine
 * available resources.</p>
 *
 * <p>
 * The {@code JPypePackage} class allows querying the contents of a package and
 * retrieving specific objects (e.g., classes or sub-packages) from the
 * package.</p>
 */
public class Package
{

  // Name of the package
  final String pkg;

  // A mapping from Python names to paths in the module/jar file system
  Map<String, URI> contents;

  int code;

  /**
   * Constructs a new {@code JPypePackage} for the specified package name.
   *
   * @param pkg The name of the Java package.
   */
  public Package(String pkg)
  {
    this.pkg = pkg;
    this.contents = PackageManager.getContentMap(pkg);
    ClassLoader classLoader = PackageManager.classloader;
    if (classLoader instanceof org.jpype.internal.DynamicClassLoader)
      this.code = ((org.jpype.internal.DynamicClassLoader) classLoader).getCode();
  }

  /**
   * Retrieves an object from the package by its name.
   *
   * <p>
   * This method is used by the importer to create attributes for `getattro`.
   * The returned object can represent various types of resources, such as
   * packages, classes, or other file types. For packages, the name is returned
   * as a string. For classes, the class object is returned.</p>
   *
   * @param name The name of the resource to retrieve.
   * @return The resource object, or {@code null} if no matching resource is
   * found.
   */
  public Object getObject(String name)
  {
    String basename = pkg + "." + Keywords.unwrap(name);
    ClassLoader cl = PackageManager.classloader;
    try
    {
      // Check if it is a package
      if (PackageManager.isPackage(basename))
      {
        return basename;
      }

      // Else probe for a class
      Class<?> cls = Class.forName(basename, false, PackageManager.classloader);
      if (Modifier.isPublic(cls.getModifiers()))
      {
        return Class.forName(basename, true, cl);
      }
    } catch (ClassNotFoundException ex)
    {
      // Continue
    }
    return null;
  }

  /**
   * Retrieves the list of contents from the Java package.
   *
   * <p>
   * This method is used to create the package `dir`, listing all resources
   * available in the package.</p>
   *
   * @return An array of resource names contained in the package.
   */
  public String[] getContents()
  {
    checkCache();
    ArrayList<String> out = new ArrayList<>();
    for (String key : contents.keySet())
    {
      URI uri = contents.get(key);
      // Skip null entries
      if (uri == null)
        continue;
      Path p = PackageManager.getPath(uri);

      // Add directories (packages)
      if (Files.isDirectory(p))
      {
        out.add(key);
      } // Add public classes
      else if (uri.toString().endsWith(".class"))
      {
        if (isPublic(p))
        {
          out.add(key);
        }
      }
    }
    return out.toArray(new String[out.size()]);
  }

  /**
   * Read exactly buf.length bytes from is, looping over short reads.
   *
   * InputStream.read(byte[]) is allowed by contract to return fewer bytes
   * than requested even before EOF; ignoring that would silently desync
   * the constant-pool walk below on any stream that does.
   *
   * @throws EOFException if the stream ends before buf is filled.
   */
  private static void readFully(InputStream is, byte[] buf) throws IOException
  {
    int off = 0;
    while (off < buf.length)
    {
      int n = is.read(buf, off, buf.length - off);
      if (n < 0)
        throw new EOFException("Unexpected end of class file");
      off += n;
    }
  }

  /**
   * Skip exactly n bytes from is, looping over short skips.
   *
   * InputStream.skip(long) is allowed by contract to skip fewer bytes than
   * requested for reasons other than EOF; a skip() that makes no progress
   * falls back to reading (and discarding) a single byte to tell a stalled
   * stream apart from one that is genuinely at EOF.
   *
   * @throws EOFException if the stream ends before n bytes are skipped.
   */
  private static void skipFully(InputStream is, long n) throws IOException
  {
    long remaining = n;
    while (remaining > 0)
    {
      long skipped = is.skip(remaining);
      if (skipped <= 0)
      {
        if (is.read() < 0)
          throw new EOFException("Unexpected end of class file");
        remaining--;
      } else
      {
        remaining -= skipped;
      }
    }
  }

  /**
   * Determines if a class is public based on its class file.
   *
   * <p>
   * This method reads the class file to check its modifier flags and determines
   * whether the class is public. Non-public classes are excluded to prevent
   * unwanted instantiation of static variables or resources.</p>
   *
   * @param p The path to the class file.
   * @return {@code true} if the class is public, {@code false} otherwise.
   */
  static boolean isPublic(Path p)
  {
    try (InputStream is = Files.newInputStream(p))
    {
      ByteBuffer buffer3 = ByteBuffer.allocate(3);

      // Check the magic number
      ByteBuffer header = ByteBuffer.allocate(4 + 2 + 2 + 2);
      readFully(is, header.array());
      ((Buffer) header).rewind();
      int magic = header.getInt();
      if (magic != (int) 0xcafebabe)
        return false;
      header.getShort(); // Skip major version
      header.getShort(); // Skip minor version
      // The count and the UTF8 lengths below are unsigned 16 bit values.
      int cpitems = header.getShort() & 0xffff; // Get number of constant pool items

      // Traverse the constant pool
      for (int i = 0; i < cpitems - 1; ++i)
      {
        readFully(is, buffer3.array());
        ((Buffer) buffer3).rewind();
        byte type = buffer3.get(); // First byte is the type

        // Advance pointer based on entry type
        switch (type)
        {
          case 1: // Strings are variable length
            skipFully(is, buffer3.getShort() & 0xffff);
            break;
          case 7:
          case 8:
          case 16:
          case 19:
          case 20:
            break;
          case 15:
            skipFully(is, 1);
            break;
          case 3:
          case 4:
          case 9:
          case 10:
          case 11:
          case 12:
          case 17:
          case 18:
            skipFully(is, 2);
            break;
          case 5:
          case 6:
            skipFully(is, 6); // Double and long are special as they take two slots
            i++;
            break;
          default:
            return false;
        }
      }

      // Get the flags
      readFully(is, buffer3.array());
      ((Buffer) buffer3).rewind();
      short flags = buffer3.getShort();
      return (flags & 1) == 1; // Public if bit zero is set
    } catch (IOException ex)
    {
      return false; // Treat as non-public if an error occurs
    }
  }

  /**
   * Checks and updates the cache for the package contents.
   *
   * <p>
   * If the class loader's state has changed, the cache is refreshed to reflect
   * the current package contents.</p>
   */
  void checkCache()
  {
    ClassLoader classLoader = PackageManager.classloader;
    if (!(classLoader instanceof org.jpype.internal.DynamicClassLoader))
      return;
    int current = ((org.jpype.internal.DynamicClassLoader) classLoader).getCode();
    if (this.code == current)
    {
      return;
    }
    this.code = current;
    this.contents = PackageManager.getContentMap(pkg);
  }
}
