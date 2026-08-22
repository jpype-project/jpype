// --- file: org/jpype/LauncherNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package org.jpype;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Path;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNotNull;
import static org.testng.Assert.assertNull;
import static org.testng.Assert.assertTrue;

/**
 * Exercises {@code Launcher}'s pure, side-effect-free logic:
 * {@link Launcher#parseVersion} and the fresh-instance getters/
 * {@code isPrepared()}, plus the private {@code checkPath} and
 * {@code findLocalWheel} path-resolution helpers (invoked via reflection,
 * since both are package-private implementation details with no public
 * entry point). The rest of {@code Launcher} (detective-probe subprocess
 * execution, pip self-healing, on-disk cache read/write, native library
 * loading) drives real subprocesses/network/filesystem access as part of
 * process bootstrap and is exercised implicitly by every other test class
 * via {@code PyTestHarness}'s one-time interpreter startup - it is not a
 * good target for isolated unit tests without dedicated mocking
 * infrastructure (see plan/Coverage.md).
 */
public class LauncherNGTest
{

  private Path tempDir;

  @AfterMethod
  public void cleanupTempDir() throws IOException
  {
    if (tempDir == null)
      return;
    try (java.util.stream.Stream<Path> stream = Files.walk(tempDir))
    {
      // Delete deepest-first so directories are empty when removed.
      stream.sorted((a, b) -> b.getNameCount() - a.getNameCount())
              .forEach(p ->
              {
                try
                {
                  Files.deleteIfExists(p);
                } catch (IOException ignored)
                {
                }
              });
    }
    tempDir = null;
  }

  private static Object invokePrivate(Object target, String name, Class<?>[] types, Object... args)
  {
    try
    {
      Method m = Launcher.class.getDeclaredMethod(name, types);
      m.setAccessible(true);
      return m.invoke(target, args);
    } catch (InvocationTargetException ex)
    {
      Throwable cause = ex.getCause();
      if (cause instanceof RuntimeException)
        throw (RuntimeException) cause;
      if (cause instanceof IOException)
        throw new java.io.UncheckedIOException((IOException) cause);
      throw new RuntimeException(cause);
    } catch (ReflectiveOperationException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  private static String checkPath(Launcher launcher, String exec)
  {
    return (String) invokePrivate(launcher, "checkPath", new Class<?>[]
    {
      String.class
    }, exec);
  }

  private static Path findLocalWheel(Launcher launcher, Path dir, String version, String arch)
  {
    return (Path) invokePrivate(launcher, "findLocalWheel", new Class<?>[]
    {
      Path.class, String.class, String.class
    }, dir, version, arch);
  }

  private Path newTempDir() throws IOException
  {
    tempDir = Files.createTempDirectory("jpype-launcher-test");
    return tempDir;
  }

  //-- checkPath --------------------------------------------------------
  @Test
  public void testCheckPathFindsExecutableOnRealPath()
  {
    // Uses the real process PATH (this is what checkPath actually reads -
    // there is no injection point for it). /usr/bin/sh (or equivalent) is
    // present and executable on every POSIX system this project targets.
    Launcher launcher = new Launcher();
    String found = checkPath(launcher, "sh");
    assertNotNull(found, "expected 'sh' to be found on PATH=" + System.getenv("PATH"));
    Path resolved = java.nio.file.Paths.get(found);
    assertTrue(Files.exists(resolved));
    assertTrue(Files.isExecutable(resolved));
    assertEquals(resolved.getFileName().toString(), "sh");
  }

  @Test
  public void testCheckPathReturnsNullForUnknownExecutable()
  {
    Launcher launcher = new Launcher();
    String found = checkPath(launcher, "jpype-definitely-not-a-real-executable-xyz123");
    assertNull(found);
  }

  //-- findLocalWheel -----------------------------------------------------
  @Test
  public void testFindLocalWheelMissingDirectoryReturnsNull() throws IOException
  {
    Launcher launcher = new Launcher();
    Path missing = newTempDir().resolve("does-not-exist");
    assertNull(findLocalWheel(launcher, missing, "1.6", "linux-x86_64"));
  }

  @Test
  public void testFindLocalWheelEmptyDirectoryReturnsNull() throws IOException
  {
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    assertNull(findLocalWheel(launcher, dir, "1.6", "linux-x86_64"));
  }

  @Test
  public void testFindLocalWheelFindsMatchingWheel() throws IOException
  {
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    Path wheel = Files.createFile(dir.resolve("JPype1-1.6.0-cp312-cp312-linux_x86_64.whl"));
    Path found = findLocalWheel(launcher, dir, "1.6", "linux_x86_64");
    assertEquals(found, wheel);
  }

  @Test
  public void testFindLocalWheelIgnoresNonWhlFiles() throws IOException
  {
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    // Contains both the version and arch substrings, but wrong extension -
    // must not match because the filter requires a ".whl" suffix.
    Files.createFile(dir.resolve("JPype1-1.6.0-linux_x86_64.tar.gz"));
    assertNull(findLocalWheel(launcher, dir, "1.6", "linux_x86_64"));
  }

  @Test
  public void testFindLocalWheelRequiresVersionMatch() throws IOException
  {
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    Files.createFile(dir.resolve("JPype1-1.5.0-cp312-cp312-linux_x86_64.whl"));
    assertNull(findLocalWheel(launcher, dir, "1.6", "linux_x86_64"));
  }

  @Test
  public void testFindLocalWheelRequiresArchMatch() throws IOException
  {
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    Files.createFile(dir.resolve("JPype1-1.6.0-cp312-cp312-win_amd64.whl"));
    assertNull(findLocalWheel(launcher, dir, "1.6", "linux_x86_64"));
  }

  @Test
  public void testFindLocalWheelPicksMatchingAmongMultipleCandidates() throws IOException
  {
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    Files.createFile(dir.resolve("JPype1-1.5.0-cp312-cp312-linux_x86_64.whl"));
    Files.createFile(dir.resolve("JPype1-1.6.0-cp312-cp312-win_amd64.whl"));
    Path match = Files.createFile(dir.resolve("JPype1-1.6.0-cp312-cp312-linux_x86_64.whl"));
    Files.createFile(dir.resolve("README.txt"));
    Path found = findLocalWheel(launcher, dir, "1.6", "linux_x86_64");
    assertEquals(found, match);
  }

  @Test
  public void testFindLocalWheelReturnsOneOfSeveralEquallyValidMatches() throws IOException
  {
    // Both files satisfy the filters - findLocalWheel's ordering isn't
    // documented/guaranteed, so just assert it deterministically picks one
    // of the two valid candidates rather than failing or returning null.
    Launcher launcher = new Launcher();
    Path dir = newTempDir();
    Path a = Files.createFile(dir.resolve("JPype1-1.6.0-cp310-cp310-linux_x86_64.whl"));
    Path b = Files.createFile(dir.resolve("JPype1-1.6.0-cp312-cp312-linux_x86_64.whl"));
    Path found = findLocalWheel(launcher, dir, "1.6", "linux_x86_64");
    assertTrue(found.equals(a) || found.equals(b));
  }

  @Test
  public void testParseVersionThreeParts()
  {
    assertEquals(Launcher.parseVersion("3.9.7"), new int[]
    {
      3, 9, 7
    });
  }

  @Test
  public void testParseVersionShortAndLong()
  {
    assertEquals(Launcher.parseVersion("3.12"), new int[]
    {
      3, 12, 0
    });
    // A 4th+ component is ignored (the array is fixed at 3 slots).
    assertEquals(Launcher.parseVersion("3.12.1.extra"), new int[]
    {
      3, 12, 1
    });
  }

  @Test
  public void testParseVersionNonNumericFallsBackToZeros()
  {
    // NumberFormatException is swallowed; whatever was parsed before the
    // bad component stays, later slots stay at their int[] default of 0.
    assertEquals(Launcher.parseVersion("abc"), new int[]
    {
      0, 0, 0
    });
  }

  @Test
  public void testFreshInstanceIsNotPrepared()
  {
    Launcher launcher = new Launcher();
    assertFalse(launcher.isPrepared());
    assertNull(launcher.getPythonExecutable());
    assertNull(launcher.getPythonLibrary());
    assertNull(launcher.getJpypeLibrary());
    assertNull(launcher.getJpypeVersion());
  }
}
