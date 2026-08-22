// --- file: org/jpype/pkg/JPypePackageNGTest.java ---
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
import java.net.URI;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Map;
import java.util.Set;
import org.jpype.internal.DynamicClassLoader;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 *
 */
public class JPypePackageNGTest
{

  public JPypePackageNGTest()
  {
  }

  @Test
  public void testGetPackage()
  {
    assertTrue(PackageManager.isPackage("java.lang"));
    Package pkg = new Package("java.lang");
    for (Map.Entry<String, URI> e : pkg.contents.entrySet())
    {
      Path p = PackageManager.getPath(e.getValue());
      Package.isPublic(p);
    }
  }

  @Test
  public void testBase()
  {
    assertTrue(PackageManager.isPackage("java"));
    Package pkg = new Package("java");
      Set<String> entries = pkg.contents.keySet();
      assertTrue(entries.contains("rmi"));
  }

  @Test
  public void testOrg()
  {
    Package pkg = new Package("org");
      Set<String> entries = pkg.contents.keySet();
      assertTrue(entries.contains("jpype"));
  }

  /**
   * Test of getObject method, of class JPypePackage.
   */
  @Test
  public void testGetObject()
  {
    Package instance = new Package("java.lang");
    Object expResult = Object.class;
    Object result = instance.getObject("Object");
    assertEquals(result, expResult);
  }

  /**
   * Test of getContents method, of class JPypePackage.
   */
  @Test
  public void testGetContents()
  {
    Package instance = new Package("java.lang");
    String[] expResult = new String[]
    {
      "AbstractMethodError", "Appendable"
    };
    String[] contents = instance.getContents();
    Arrays.sort(contents);
    String[] result = Arrays.copyOfRange(contents, 0, 2);
    assertEquals(result, expResult);
  }

  @Test
  public void testGetObjectReturnsSubpackageName()
  {
    Package instance = new Package("java");
    Object result = instance.getObject("lang");
    assertEquals(result, "java.lang");
  }

  @Test
  public void testGetObjectNotFoundReturnsNull()
  {
    Package instance = new Package("java.lang");
    assertNull(instance.getObject("ThisClassDoesNotExistXYZ"));
  }

  @Test
  public void testGetContentsSkipsNullEntries()
  {
    Package instance = new Package("java.lang");
    instance.contents.put("__coverageTestNullEntry__", null);
    for (String s : instance.getContents())
      assertNotEquals(s, "__coverageTestNullEntry__");
  }

  @Test
  public void testIsPublicBadMagicReturnsFalse() throws IOException
  {
    Path tmp = Files.createTempFile("bogus", ".class");
    try
    {
      Files.write(tmp, new byte[]
      {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0
      });
      assertFalse(Package.isPublic(tmp));
    } finally
    {
      Files.deleteIfExists(tmp);
    }
  }

  /**
   * Hand-built minimal class-file header: real magic/cpcount, then a single
   * constant-pool entry whose tag (2) isn't one of the recognized cases -
   * exercises the {@code default: return false} branch of the constant-pool
   * scan without needing a real malformed .class off disk.
   */
  @Test
  public void testIsPublicUnsupportedConstantPoolTagReturnsFalse() throws IOException
  {
    Path tmp = Files.createTempFile("bogus2", ".class");
    try
    {
      byte[] data = new byte[]
      {
        (byte) 0xCA, (byte) 0xFE, (byte) 0xBA, (byte) 0xBE,
        0, 0, 0, 0x34,
        0, 2,
        2, 0, 0
      };
      Files.write(tmp, data);
      assertFalse(Package.isPublic(tmp));
    } finally
    {
      Files.deleteIfExists(tmp);
    }
  }

  @Test
  public void testCheckCacheRefreshesWhenDynamicClassLoaderCodeChanges() throws Exception
  {
    ClassLoader original = PackageManager.classloader;
    DynamicClassLoader dyn = new DynamicClassLoader(original);
    try
    {
      PackageManager.setClassLoader(dyn);
      Package instance = new Package("java.lang");
      int codeBefore = instance.code;
      assertEquals(codeBefore, dyn.getCode());

      dyn.addURL(new java.io.File(".").toURI().toURL());
      assertNotEquals(dyn.getCode(), codeBefore);

      instance.checkCache();
      assertEquals(instance.code, dyn.getCode());
    } finally
    {
      PackageManager.setClassLoader(original);
    }
  }

}
