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
import java.io.InputStream;
import java.lang.reflect.Modifier;
import java.net.URI;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Map;
import org.jpype.JPypeClassLoader;
import org.jpype.JPypeContext;
import org.jpype.JPypeKeywords;

/**
 * Represents a Java package in JPype.
 *
 * <p>
 * This class provides the directory structure and attributes for a Java
 * package, enabling JPype imports. Most of the heavy lifting is done by the
 * {@link JPypePackageManager}, which acts as a class loader to determine
 * available resources.</p>
 *
 * <p>
 * The {@code JPypePackage} class allows querying the contents of a package and
 * retrieving specific objects (e.g., classes or sub-packages) from the
 * package.</p>
 */
public class JPypePackage
{

  // Name of the package
  final String pkg;

  // A mapping from Python names to paths in the module/jar file system
  Map<String, URI> contents;

  int code;
  private final JPypeClassLoader classLoader;

  /**
   * Constructs a new {@code JPypePackage} for the specified package name.
   *
   * @param pkg The name of the Java package.
   */
  public JPypePackage(String pkg)
  {
    this.pkg = pkg;
    this.contents = JPypePackageManager.getContentMap(pkg);
    this.classLoader = ((JPypeClassLoader) (JPypeContext.getInstance().getClassLoader()));
    this.code = classLoader.getCode();
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
    String basename = pkg + "." + JPypeKeywords.unwrap(name);
    ClassLoader cl = JPypeContext.getInstance().getClassLoader();
    try
    {
      // Check if it is a package
      if (JPypePackageManager.isPackage(basename))
      {
        return basename;
      }

      // Else probe for a class
      Class<?> cls = Class.forName(basename, false, JPypeContext.getInstance().getClassLoader());
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
      Path p = JPypePackageManager.getPath(uri);

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
      is.read(header.array());
      ((Buffer) header).rewind();
      int magic = header.getInt();
      if (magic != (int) 0xcafebabe)
        return false;
      header.getShort(); // Skip major version
      header.getShort(); // Skip minor version
      short cpitems = header.getShort(); // Get number of constant pool items

      // Traverse the constant pool
      for (int i = 0; i < cpitems - 1; ++i)
      {
        is.read(buffer3.array());
        ((Buffer) buffer3).rewind();
        byte type = buffer3.get(); // First byte is the type

        // Advance pointer based on entry type
        switch (type)
        {
          case 1: // Strings are variable length
            is.skip(buffer3.getShort());
            break;
          case 7:
          case 8:
          case 16:
          case 19:
          case 20:
            break;
          case 15:
            is.skip(1);
            break;
          case 3:
          case 4:
          case 9:
          case 10:
          case 11:
          case 12:
          case 17:
          case 18:
            is.skip(2);
            break;
          case 5:
          case 6:
            is.skip(6); // Double and long are special as they take two slots
            i++;
            break;
          default:
            return false;
        }
      }

      // Get the flags
      is.read(buffer3.array());
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
    int current = classLoader.getCode();
    if (this.code == current)
    {
      return;
    }
    this.code = current;
    this.contents = JPypePackageManager.getContentMap(pkg);
  }
}
