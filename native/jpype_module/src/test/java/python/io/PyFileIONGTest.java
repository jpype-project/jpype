// --- file: python/io/PyFileIONGTest.java ---
package python.io;

import static org.testng.Assert.*;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;
import python.lang.PyBytes;
import python.lang.PyTestHarness;

public class PyFileIONGTest extends PyTestHarness
{

  private Path tempFile;

  @AfterMethod
  public void cleanup() throws IOException
  {
    if (tempFile != null)
      Files.deleteIfExists(tempFile);
  }

  @Test
  public void testWriteAndReadBack() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".bin");
    String path = tempFile.toAbsolutePath().toString();

    PyFileIO writer = IO.using(context).fileIO(path, "wb");
    int written = writer.write(context.bytesFromHex("68656c6c6f"));
    writer.close();

    assertEquals(written, 5);

    PyFileIO reader = IO.using(context).fileIO(path, "rb");
    PyBytes read = reader.readall();
    reader.close();

    assertEquals(read.decode("utf-8", null).toString(), "hello");
  }

  @Test
  public void testNameAndMode() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".bin");
    String path = tempFile.toAbsolutePath().toString();

    PyFileIO instance = IO.using(context).fileIO(path, "rb");

    assertEquals(instance.name(), path);
    assertEquals(instance.mode(), "rb");

    instance.close();
  }

  @Test
  public void testReadinto() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".bin");
    Files.write(tempFile, "abc".getBytes("UTF-8"));
    String path = tempFile.toAbsolutePath().toString();

    PyFileIO instance = IO.using(context).fileIO(path, "rb");
    python.lang.PyByteArray buffer = context.bytearray(3);
    int n = instance.readinto(buffer);
    instance.close();

    assertEquals(n, 3);
    assertEquals(buffer.decode(context.str("utf-8"), null).toString(), "abc");
  }

  @Test
  public void testClose() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".bin");
    PyFileIO instance = IO.using(context).fileIO(tempFile.toAbsolutePath().toString(), "wb");

    instance.close();

    assertTrue(instance.closed());
  }
}
