/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.jpype.pkg;

/**
 *
 * @author nelson85
 */
public class ListPackage
{

  public static void main(String[] args)
  {
    JPypePackage pkg = new JPypePackage("java", JPypePackageManager.getContentMap("java"));
    System.out.println(pkg.contents.size());
    for (String s : pkg.getContents())
    {
      System.out.println(s);
    }
  }

}
