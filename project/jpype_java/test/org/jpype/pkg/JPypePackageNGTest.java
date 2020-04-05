/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.jpype.pkg;

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
    JPypePackage pkg = new JPypePackage("java.lang", JPypePackageManager.getContentMap("java.lang"));
    for (Map.Entry<String, Path> e : pkg.contents.entrySet())
    {
      JPypePackage.isPublic(e.getValue());
    }
  }

  @Test
  public void testBase()
  {
    assertTrue(JPypePackageManager.isPackage("java"));
    JPypePackage pkg = new JPypePackage("java", JPypePackageManager.getContentMap("java"));
    for (Map.Entry<String, Path> e : pkg.contents.entrySet())
    {
      System.out.println(e.getKey());
    }
  }

  @Test
  public void testOrg()
  {
    assertTrue(JPypePackageManager.isPackage("org"));
    JPypePackage pkg = new JPypePackage("org", JPypePackageManager.getContentMap("org"));
    for (Map.Entry<String, Path> e : pkg.contents.entrySet())
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
    JPypePackage instance = new JPypePackage("java.lang", JPypePackageManager.getContentMap("java.lang"));
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
    JPypePackage instance = new JPypePackage("java.lang", JPypePackageManager.getContentMap("java.lang"));
    String[] expResult = new String[]
    {
      "Enum", "ClassValue", "String"
    };
    String[] result = Arrays.copyOfRange(instance.getContents(), 0, 3);
    assertEquals(result, expResult);
  }

}
