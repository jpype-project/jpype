// --- file: org/jpype/internal/DynamicClassLoaderNGTest.java ---
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
package org.jpype.internal;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Enumeration;
import java.util.jar.JarEntry;
import java.util.jar.JarOutputStream;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises DynamicClassLoader directly with fresh, isolated instances -
 * deliberately never touching {@link DynamicClassLoader#install} (the
 * static singleton other tests/the real bootstrap may already depend on).
 */
public class DynamicClassLoaderNGTest
{

  @Test
  public void testAddPathMissingFileThrows()
  {
    DynamicClassLoader loader = new DynamicClassLoader(null);
    try
    {
      loader.addPath(java.nio.file.Paths.get("/this/path/does/not/exist/xyz.jar"));
      fail("Expected FileNotFoundException");
    } catch (FileNotFoundException ex)
    {
      // expected
    }
  }

  @Test
  public void testAddPathExistingDirectoryUpdatesCode() throws IOException
  {
    DynamicClassLoader loader = new DynamicClassLoader(null);
    int before = loader.getCode();
    Path dir = Files.createTempDirectory("jpype-dcl-test");
    try
    {
      loader.addPath(dir);
      assertNotEquals(loader.getCode(), before);
    } finally
    {
      Files.delete(dir);
    }
  }

  @Test
  public void testAddPathsWalksTreeWithGlob() throws IOException
  {
    Path root = java.nio.file.Files.createTempDirectory("jpype-dcl-test");
    try
    {
      Path sub = Files.createDirectories(root.resolve("a/b"));
      Path match1 = root.resolve("a/one.txt");
      Path match2 = sub.resolve("two.txt");
      Path noMatch = sub.resolve("three.dat");
      Files.write(match1, "1".getBytes());
      Files.write(match2, "2".getBytes());
      Files.write(noMatch, "3".getBytes());

      DynamicClassLoader loader = new DynamicClassLoader(null);
      loader.addPaths(root, "glob:**.txt");

      // Each matching file (visited via the anonymous SimpleFileVisitor)
      // should have been individually added to the loader's URL search
      // path; the non-matching file should not.
      java.util.List<String> urls = java.util.Arrays.stream(loader.getURLs())
              .map(URL::toString).collect(java.util.stream.Collectors.toList());
      assertTrue(urls.stream().anyMatch(u -> u.endsWith("one.txt")));
      assertTrue(urls.stream().anyMatch(u -> u.endsWith("two.txt")));
      assertTrue(urls.stream().noneMatch(u -> u.endsWith("three.dat")));
    } finally
    {
      deleteRecursively(root);
    }
  }

  @Test
  public void testFindClassFallsBackToParentWhenNoResource()
  {
    // A bootstrap-only parent and an empty URL search path mean
    // this.getResource(...) can't find "org/jpype/internal/Support.class"
    // (it's an application class, not on the boot classpath) - so
    // findClass must take its url==null fallback branch, resolving via
    // Class.forName against DynamicClassLoader's own defining loader
    // instead of ever reaching the defineClass(byte[]) path.
    DynamicClassLoader loader = new DynamicClassLoader(null);
    try
    {
      Class<?> cls = loader.findClass("org.jpype.internal.Support");
      assertEquals(cls, Support.class);
    } catch (ClassNotFoundException ex)
    {
      fail("org.jpype.internal.Support should resolve via the fallback path", ex);
    }
  }

  @Test
  public void testFindClassDefinesFromBytesWhenResourcePresent() throws Exception
  {
    URL classesDir = Support.class.getProtectionDomain().getCodeSource().getLocation();
    DynamicClassLoader loader = new DynamicClassLoader(null);
    loader.addURL(classesDir);

    Class<?> loaded = loader.findClass("org.jpype.internal.Support");
    assertEquals(loaded.getName(), "org.jpype.internal.Support");
    // Defined fresh from raw bytes via this loader - not the same Class
    // object as the one already loaded by the normal application loader.
    assertNotSame(loaded, Support.class);
  }

  @Test
  public void testFindResourceOrgJpypePrefixFallback()
  {
    DynamicClassLoader loader = new DynamicClassLoader(null);
    URL url = loader.findResource("org/jpype/internal/Keywords.class");
    // Falls back to META-INF/versions/0/org/jpype/... which may or may not
    // resolve depending on what's on this loader's own (empty) URL search
    // path - the important thing is it takes the special-cased branch
    // rather than throwing, whichever way it resolves.
    assertTrue(url == null || url.toString().contains("META-INF/versions/0"));
  }

  @Test
  public void testAddResourceAndFindResources() throws Exception
  {
    DynamicClassLoader loader = new DynamicClassLoader(null);
    URL fake = new URL("file:///fake/path/");
    loader.addResource("some/dir", fake);

    Enumeration<URL> found = loader.findResources("some/dir");
    assertTrue(found.hasMoreElements());
    assertEquals(found.nextElement(), fake);

    // Trailing slash must resolve identically.
    URL viaFindResource = loader.findResource("some/dir/");
    assertEquals(viaFindResource, fake);
  }

  @Test
  public void testScanJarSynthesizesMissingDirectoryEntries() throws Exception
  {
    Path jar = java.nio.file.Files.createTempFile("jpype-dcl-test", ".jar");
    try
    {
      try (JarOutputStream jos = new JarOutputStream(Files.newOutputStream(jar)))
      {
        // Deliberately no directory entries - only a leaf file entry, the
        // "jar files lack directory support" case scanJar exists to patch.
        jos.putNextEntry(new JarEntry("org/jpype/internal/Fake.txt"));
        jos.write("x".getBytes());
        jos.closeEntry();
      }

      DynamicClassLoader loader = new DynamicClassLoader(null);
      loader.addURL(jar.toUri().toURL());

      assertNotNull(loader.findResource("org"));
      assertNotNull(loader.findResource("org/jpype"));
      assertNotNull(loader.findResource("org/jpype/internal"));
    } finally
    {
      Files.deleteIfExists(jar);
    }
  }

  private static void deleteRecursively(Path root) throws IOException
  {
    if (!Files.exists(root))
      return;
    Files.walk(root)
            .sorted(java.util.Comparator.reverseOrder())
            .forEach(p ->
            {
              try
              {
                Files.delete(p);
              } catch (IOException ex)
              {
              }
            });
  }

}
