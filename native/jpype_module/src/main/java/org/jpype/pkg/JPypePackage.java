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
 * Representation of a JPackage in Java.
 *
 * This provides the dir and attributes for a JPackage and by extension jpype
 * imports. Almost all of the actual work happens in the PackageManager which
 * acts like the classloader to figure out what resource are available.
 *
 */
public class JPypePackage
{

  // Name of the package
  final String pkg;
  // A mapping from Python names into Paths into the module/jar file system.
  Map<String, URI> contents;
  int code;
  private final JPypeClassLoader classLoader;

  public JPypePackage(String pkg)
  {
    this.pkg = pkg;
    this.contents = JPypePackageManager.getContentMap(pkg);
    this.classLoader = ((JPypeClassLoader) (JPypeContext.getInstance().getClassLoader()));
    this.code = classLoader.getCode();
  }

  /**
   * Get an object from the package.
   *
   * This is used by the importer to create the attributes for `getattro`. The
   * type returned is polymorphic. We can potentially support any type of
   * resource (package, classes, property files, xml, data, etc). But for now we
   * are primarily interested in packages and classes. Packages are returned as
   * strings as loading the package info is not guaranteed to work. Classes are
   * returned as classes which are immediately converted into Python wrappers.
   * We can return other resource types so long as they have either a wrapper
   * type to place the instance into an Python object directly or a magic
   * wrapper which will load the resource into a Python object type.
   *
   * This should match the acceptable types in getContents so that everything in
   * the `dir` is also an attribute of JPackage.
   *
   * @param name is the name of the resource.
   * @return the object or null if no resource is found with a matching name.
   */
  public Object getObject(String name)
  {
    // We can't use the url contents as the contents may be incomplete due
    // to bugs in the JVM classloaders.  Instead we will have to probe.
    String basename = pkg + "." + JPypeKeywords.unwrap(name);
    ClassLoader cl = JPypeContext.getInstance().getClassLoader();
    try
    {
      // Check if it a package
      if (JPypePackageManager.isPackage(basename))
      {
        return basename;
      }

      // Else probe for a class.
      Class<?> cls = Class.forName(basename, false, JPypeContext.getInstance().getClassLoader());
      if (Modifier.isPublic(cls.getModifiers()))
        return Class.forName(basename, true, cl);
    } catch (ClassNotFoundException ex)
    {
      // Continue
    }
    return null;
  }

  /**
   * Get a list of contents from a Java package.
   *
   * This will be used when creating the package `dir`
   *
   * @return
   */
  public String[] getContents()
  {
    checkCache();
    ArrayList<String> out = new ArrayList<>();
    for (String key : contents.keySet())
    {
      URI uri = contents.get(key);
      // If there is anything null, then skip it.
      if (uri == null)
        continue;
      Path p = JPypePackageManager.getPath(uri);

      // package are acceptable
      if (Files.isDirectory(p))
        out.add(key);

      // classes must be public
      else if (uri.toString().endsWith(".class"))
      {
        // Make sure it is public
        if (isPublic(p))
          out.add(key);
      }
    }
    return out.toArray(new String[out.size()]);
  }

  /**
   * Determine if a class is public.
   *
   * This checks if a class file contains a public class. When importing classes
   * we do not want to instantiate a class which is not public as it may result
   * in instantiation of static variables or unwanted class resources. The only
   * alternative is to read the class file and get the class modifier flags.
   * Unfortunately, the developers of Java were rather stingy on their byte
   * allocation and thus the field we want is not in the header but rather
   * buried after the constant pool. Further as they didn't give the actual size
   * of the tables in bytes, but rather in entries, that means we have to parse
   * the whole table just to get the access flags after it.
   *
   * @param p
   * @return
   */
  static boolean isPublic(Path p)
  {
    try (InputStream is = Files.newInputStream(p))
    {
      // Allocate a three byte buffer for traversing the constant pool.
      // The minumum entry is a byte for the type and 2 data bytes.  We
      // will read these three bytes and then based on the type advance
      // the read pointer to the next entry.
      ByteBuffer buffer3 = ByteBuffer.allocate(3);

      // Check the magic
      ByteBuffer header = ByteBuffer.allocate(4 + 2 + 2 + 2);
      is.read(header.array());
      ((Buffer) header).rewind();
      int magic = header.getInt();
      if (magic != (int) 0xcafebabe)
        return false;
      header.getShort(); // skip major
      header.getShort(); // skip minor
      short cpitems = header.getShort(); // get the number of items

      // Traverse the cp pool
      for (int i = 0; i < cpitems - 1; ++i)
      {
        is.read(buffer3.array());
        ((Buffer) buffer3).rewind();
        byte type = buffer3.get(); // First byte is the type

        // Now based on the entry type we will advance the pointer
        switch (type)
        {
          case 1:  // Strings are variable length
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
            is.skip(6); // double and long are special as they are double entries
            i++; // long and double take two slots
            break;
          default:
            return false;
        }
      }

      // Get the flags
      is.read(buffer3.array());
      ((Buffer) buffer3).rewind();
      short flags = buffer3.getShort();
      return (flags & 1) == 1; // it is public if bit zero is set
    } catch (IOException ex)
    {
      return false; // If anything goes wrong then it won't be considered a public class.
    }
  }

  void checkCache()
  {
    int current = classLoader.getCode();
    if (this.code == current)
      return;
    this.code = current;
    this.contents = JPypePackageManager.getContentMap(pkg);
  }

}
