// --- file: org/jpype/pkg/PackageManagerNGTest.java ---
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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.file.FileSystemNotFoundException;
import java.util.Map;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PackageManagerNGTest
{

  @Test(expectedExceptions = FileSystemNotFoundException.class)
  public void testGetPathThrowsForUnknownScheme() throws URISyntaxException
  {
    PackageManager.getPath(new URI("definitely-not-a-real-scheme://something"));
  }

  @Test
  public void testGetFileSystemProviderThrowsForUnknownScheme() throws Exception
  {
    Method m = PackageManager.class.getDeclaredMethod("getFileSystemProvider", String.class);
    m.setAccessible(true);
    try
    {
      m.invoke(null, "definitely-not-a-real-scheme");
      fail("Expected FileSystemNotFoundException");
    } catch (InvocationTargetException ex)
    {
      assertTrue(ex.getCause() instanceof FileSystemNotFoundException);
    }
  }

  @Test
  public void testIsPackageNonexistentReturnsFalse()
  {
    assertFalse(PackageManager.isPackage("this.does.not.exist.zzcoverage"));
  }

  /**
   * A 5-segment dotted name (&gt; 3 path segments once split on '/')
   * exercises the search-key-truncation branch shared by
   * {@code isModulePackage}/{@code getModuleContents} - the inner class
   * itself isn't a package, but the truncated lookup against its enclosing
   * module still has to run to find that out.
   */
  @Test
  public void testIsPackageDeepNestedNameReturnsFalse()
  {
    assertFalse(PackageManager.isPackage("java.lang.invoke.MethodHandles.Lookup"));
  }

  @Test
  public void testGetContentMapDeepNestedNameHandled()
  {
    Map<String, URI> contents = PackageManager.getContentMap(
            "java.lang.invoke.MethodHandles.Lookup");
    assertNotNull(contents);
  }

  /**
   * {@code zzcoveragetestpkg} (see src/test/resources) lives only on the
   * plain test classpath - not in any JDK module and not in the org.jpype
   * named module. Calls the private {@code isJarPackage} directly (via
   * reflection) rather than going through {@code isPackage}, since once any
   * bridge-startup test has run in this suite, {@code NativeContext}
   * permanently swaps in a {@code DynamicClassLoader} (a process-wide
   * singleton installed via {@code DynamicClassLoader.install}) as
   * {@code classloader}, and going through the public {@code isPackage}
   * entry point left it ambiguous whether {@code isModulePackage}/
   * {@code isNamedModulePackage} or this method actually matched.
   */
  @Test
  public void testIsJarPackageFindsPlainClasspathDirectory() throws Exception
  {
    ClassLoader original = PackageManager.classloader;
    try
    {
      PackageManager.setClassLoader(PackageManagerNGTest.class.getClassLoader());
      Method m = PackageManager.class.getDeclaredMethod("isJarPackage", String.class);
      m.setAccessible(true);
      assertEquals(m.invoke(null, "zzcoveragetestpkg"), Boolean.TRUE);
    } finally
    {
      PackageManager.setClassLoader(original);
    }
  }
}
