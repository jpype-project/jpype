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

import java.net.URI;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Map;
import java.util.Set;
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

}
