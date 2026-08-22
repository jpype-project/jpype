// --- file: python/io/PyBufferedAndTextWrapperNGTest.java ---
package python.io;

import static org.testng.Assert.*;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyBufferedAndTextWrapperNGTest extends PyTestHarness
{

  private Path tempFile;

  @AfterMethod
  public void cleanup() throws IOException
  {
    if (tempFile != null)
      Files.deleteIfExists(tempFile);
  }

  @Test
  public void testBufferedWriterAndReader() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".bin");
    String path = tempFile.toAbsolutePath().toString();

    PyBufferedWriter writer = (PyBufferedWriter) context.eval(
            "open('" + path + "', 'wb')");
    writer.write(context.bytesFromHex("68656c6c6f"));
    writer.close();

    PyBufferedReader reader = (PyBufferedReader) context.eval(
            "open('" + path + "', 'rb')");
    python.lang.PyBytes read = reader.read();
    reader.close();

    assertEquals(read.decode("utf-8", null).toString(), "hello");
  }

  @Test
  public void testBufferedRandom() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".bin");
    String path = tempFile.toAbsolutePath().toString();

    PyBufferedRandom instance = (PyBufferedRandom) context.eval(
            "open('" + path + "', 'rb+')");

    assertNotNull(instance);
    assertTrue(instance.readable());
    assertTrue(instance.writable());
    instance.close();
  }

  @Test
  public void testTextIOWrapper() throws IOException
  {
    tempFile = Files.createTempFile("jpype-io-", ".txt");
    String path = tempFile.toAbsolutePath().toString();

    PyTextIOWrapper writer = (PyTextIOWrapper) context.eval(
            "open('" + path + "', 'w')");
    writer.write("hello");
    writer.close();

    PyTextIOWrapper reader = (PyTextIOWrapper) context.eval(
            "open('" + path + "', 'r')");
    assertFalse(reader.line_buffering());
    assertEquals(reader.read().toString(), "hello");
    reader.close();
  }
}
