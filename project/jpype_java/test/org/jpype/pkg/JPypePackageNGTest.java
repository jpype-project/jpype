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
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 *
 * @author nelson85
 */
public class JPypePackageNGTest
{

  public JPypePackageNGTest()
  {
  }

  @Test
  public void testGetPackage()
  {
    assertTrue(JPypePackageManager.isPackage("java.lang"));
    JPypePackage pkg = new JPypePackage("java.lang");
    for (Map.Entry<String, URI> e : pkg.contents.entrySet())
    {
      Path p = JPypePackageManager.getPath(e.getValue());
      JPypePackage.isPublic(p);
    }
  }

  @Test
  public void testBase()
  {
    assertTrue(JPypePackageManager.isPackage("java"));
    JPypePackage pkg = new JPypePackage("java");
    for (Map.Entry<String, URI> e : pkg.contents.entrySet())
    {
      System.out.println(e.getKey());
    }
  }

  @Test
  public void testOrg()
  {
    assertTrue(JPypePackageManager.isPackage("org"));
    JPypePackage pkg = new JPypePackage("org");
    for (Map.Entry<String, URI> e : pkg.contents.entrySet())
    {
      System.out.println(e.getKey());
    }
  }

  /**
   * Test of getObject method, of class JPypePackage.
   */
  @Test
  public void testGetObject()
  {
    System.out.println("getObject");
    JPypePackage instance = new JPypePackage("java.lang");
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
    System.out.println("getContents");
    JPypePackage instance = new JPypePackage("java.lang");
    String[] expResult = new String[]
    {
      "Enum", "ClassValue", "String"
    };
    String[] result = Arrays.copyOfRange(instance.getContents(), 0, 3);
    assertEquals(result, expResult);
  }

}
