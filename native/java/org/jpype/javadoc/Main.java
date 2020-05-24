/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.jpype.javadoc;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Paths;
import org.w3c.dom.Node;

/**
 *
 * @author nelson85
 */
public class Main
{
  public static void main(String[] args) throws IOException
  {
    // html = JClass("org.jpype.html.Html")
    //hw = JClass("org.jpype.html.HtmlWriter")
    JavadocZip jdz = new JavadocZip(Paths.get("jdk-8u251-docs-all.zip"));
    JavadocTransformer jdf = new JavadocTransformer();
    JavadocRenderer jdr = new JavadocRenderer();
    Class p = java.awt.Button.class;
    InputStream jis = jdz.getInputStream(p);
    Javadoc jd = JavadocExtractor.extractStream(jis);

    for (Node c : jd.methods)
    {
      Node n = jdf.transformMember(c);
      System.out.println(jdr.renderMember(n));
      System.out.println(jdr.memberName);
    }

  }
}
