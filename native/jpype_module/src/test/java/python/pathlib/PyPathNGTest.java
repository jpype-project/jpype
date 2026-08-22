// --- file: python/pathlib/PyPathNGTest.java ---
package python.pathlib;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyPathNGTest extends PyTestHarness
{

  @Test
  public void testConstructionAndParts()
  {
    PyPath p = Pathlib.using(context).path("/a/b", "c.txt");

    assertEquals(p.name(), "c.txt");
    assertEquals(p.stem(), "c");
    assertEquals(p.suffix(), ".txt");
    assertEquals(p.toString(), "/a/b/c.txt");
  }

  @Test
  public void testSuffixes()
  {
    PyPath p = Pathlib.using(context).path("archive.tar.gz");

    assertEquals(p.suffixes().size(), 2);
    assertEquals(p.suffixes().get(0).toString(), ".tar");
    assertEquals(p.suffixes().get(1).toString(), ".gz");
  }

  @Test
  public void testPartsAndParent()
  {
    PyPath p = Pathlib.using(context).path("/a/b/c.txt");

    assertEquals(p.parts().size(), 4);
    assertEquals(p.parent().toString(), "/a/b");
  }

  @Test
  public void testAnchorDriveRoot()
  {
    PyPath abs = Pathlib.using(context).path("/a/b");
    PyPath rel = Pathlib.using(context).path("a/b");

    assertEquals(abs.anchor(), "/");
    assertEquals(abs.root(), "/");
    assertEquals(abs.drive(), "");
    assertTrue(abs.isAbsolute());
    assertFalse(rel.isAbsolute());
  }

  @Test
  public void testAsPosix()
  {
    PyPath p = Pathlib.using(context).path("/a/b");

    assertEquals(p.asPosix(), "/a/b");
  }

  @Test
  public void testJoin()
  {
    PyPath p = Pathlib.using(context).path("/a");

    assertEquals(p.join("b", "c.txt").toString(), "/a/b/c.txt");
  }

  @Test
  public void testWithNameAndSuffix()
  {
    PyPath p = Pathlib.using(context).path("/a/b.txt");

    assertEquals(p.withName("c.txt").toString(), "/a/c.txt");
    assertEquals(p.withSuffix(".md").toString(), "/a/b.md");
  }

  @Test
  public void testMatches()
  {
    PyPath p = Pathlib.using(context).path("/a/b.txt");

    assertTrue(p.matches("*.txt"));
    assertFalse(p.matches("*.md"));
  }

  @Test
  public void testRelativeToAndIsRelativeTo()
  {
    PyPath base = Pathlib.using(context).path("/a");
    PyPath child = Pathlib.using(context).path("/a/b/c");

    assertTrue(child.isRelativeTo(base));
    assertEquals(child.relativeTo(base).toString(), "b/c");
  }

  @Test
  public void testCompareToAndEquals()
  {
    Pathlib pathlib = Pathlib.using(context);
    PyPath a = pathlib.path("/a");
    PyPath b = pathlib.path("/b");
    PyPath aAgain = pathlib.path("/a");

    assertTrue(a.compareTo(b) < 0);
    assertTrue(b.compareTo(a) > 0);
    assertEquals(a.compareTo(aAgain), 0);
    assertEquals(a, aAgain);
  }

  @Test
  public void testFilesystemPredicatesOnRealFile() throws IOException
  {
    File tmp = File.createTempFile("jpype-pathlib-test", ".txt");
    tmp.deleteOnExit();
    PyPath p = Pathlib.using(context).pathFromFile(tmp);

    assertTrue(p.exists());
    assertTrue(p.isFile());
    assertFalse(p.isDirectory());
    assertFalse(p.isSymlink());
  }

  @Test
  public void testFilesystemPredicatesOnMissingPath()
  {
    PyPath p = Pathlib.using(context).path("/definitely/does/not/exist/xyz");

    assertFalse(p.exists());
    assertFalse(p.isFile());
    assertFalse(p.isDirectory());
  }

  @Test
  public void testToNioPathAndToFile()
  {
    PyPath p = Pathlib.using(context).path("/a/b.txt");

    assertEquals(p.toNioPath(), java.nio.file.Paths.get("/a/b.txt"));
    assertEquals(p.toFile(), new File("/a/b.txt"));
  }

  @Test
  public void testPathFromNioPath() throws IOException
  {
    java.nio.file.Path nioPath = Files.createTempDirectory("jpype-pathlib-test");
    PyPath p = Pathlib.using(context).pathFromNioPath(nioPath);

    assertTrue(p.isDirectory());
    assertEquals(p.toString(), nioPath.toString());
  }

}
