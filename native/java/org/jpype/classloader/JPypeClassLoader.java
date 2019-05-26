/*
 * Copyright 2018, Karl Nelson
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
package org.jpype.classloader;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.TreeMap;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;

/**
 * Specialized class loader for jpype resources.
 * <p>
 * Loader to convert the internally stored resources into java classes. This
 * prevents class load order problems when there are class dependencies.
 * <p>
 */
public class JPypeClassLoader extends ClassLoader
{
  static private JPypeClassLoader instance;
  private TreeMap<String, byte[]> map = new TreeMap<>();

  /**
   * Get the class loader.
   *
   * @return the singleton class loader.
   */
  public static JPypeClassLoader getInstance()
  {
    if (instance == null)
    {
      JPypeClassLoader.instance = new JPypeClassLoader(getSystemClassLoader());
    }
    return instance;
  }

  private JPypeClassLoader(ClassLoader parent)
  {
    super(parent);
  }

  /**
   * Add a class to the class loader.
   * <p>
   * This can be called from within python to add a class to the Java JVM.
   *
   * @param name is the name of the class.
   * @param code is the byte code.
   */
  public void importClass(String name, byte[] code)
  {
    if (name.endsWith(".class"))
      name = name.substring(0, name.length() - 6);
    name = name.replaceAll("/", ".");
    map.put(name, code);
  }

  /**
   * Import a jar from memory into the class loader.
   * <p>
   * Does not handle unknown jar entry lengths.
   *
   * @param bytes
   */
  public void importJar(byte[] bytes)
  {
    try (JarInputStream is = new JarInputStream(new ByteArrayInputStream(bytes)))
    {
      while (true)
      {
        JarEntry nextEntry = is.getNextJarEntry();
        if (nextEntry == null)
          break;

        // Skip directories and other non-class resources
        long size = nextEntry.getSize();
        if (size <= 0)
          continue;

        byte[] data = new byte[(int) size];
        int total = 0;

        // Read the contents.
        while (true)
        {
          int r = is.read(data, total, data.length - total);
          total += r;
          if (r == 0 || total == size)
            break;
        }

        // Store all classes we find
        String name = nextEntry.getName();
        if (name.endsWith(".class"))
        {
          importClass(name, data);
        }
      }
    } catch (IOException ex)
    {
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
    byte[] data = map.get(name);
    if (data == null)
    {
      // Call the default implementation, throws ClassNotFoundException
      return super.findClass(name);
    }

    Class cls = defineClass(name, data, 0, data.length);
    if (cls == null)
      throw new ClassFormatError("Class load was null");
    return cls;
  }
}
