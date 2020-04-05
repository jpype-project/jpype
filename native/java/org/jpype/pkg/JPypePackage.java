/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.jpype.pkg;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Map;

/**
 *
 * @author nelson85
 */
public class JPypePackage
{

  final String pkg;
  final Map<String, Path> contents;

  public JPypePackage(String pkg, Map<String, Path> contents)
  {
    this.pkg = pkg;
    this.contents = contents;
  }

  public Object getObject(String name)
  {
    Path p = contents.get(name);
    if (p == null)
      return null;
    if (Files.isDirectory(p))
      return p.toString().replace("/", ".");
    if (p.toString().endsWith(".class"))
    {
      try
      {
        if (!isPublic(p))
          return null;
        return Class.forName(pkg + "." + name);
      } catch (ClassNotFoundException ex)
      {
        return null;
      }
    }
    return null;
  }

  public String[] getContents()
  {
    ArrayList<String> out = new ArrayList<>();
    for (String key : contents.keySet())
    {
      Object o = this.getObject(key);
      if (o != null)
        out.add(key);
    }
    return out.toArray(new String[out.size()]);
  }

  static boolean isPublic(Path p)
  {
    try (InputStream is = Files.newInputStream(p))
    {
      ByteBuffer buffer3 = ByteBuffer.allocate(3);

      // Check the magic
      ByteBuffer header = ByteBuffer.allocate(4 + 2 + 2 + 2);
      is.read(header.array());
      header.rewind();
      int magic = header.getInt();
      if (magic != (int) 0xcafebabe)
        return false;
      header.getShort(); // skip major
      header.getShort(); // skip minor
      int cpitems = header.getShort();

      // Traverse the cp pool
      for (int i = 0; i < cpitems - 1; ++i)
      {
        is.read(buffer3.array());
        buffer3.rewind();
        int type = buffer3.get();
        switch (type)
        {
          case 1:
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
            is.skip(6);
            i++; // long and double take two slots
            break;
          default:
            return false;
        }
      }

      // Get the flags
      is.read(buffer3.array());
      buffer3.rewind();
      int flags = buffer3.getShort();
      return (flags & 1) == 1;
    } catch (IOException ex)
    {
      return false;
    }
  }

}
