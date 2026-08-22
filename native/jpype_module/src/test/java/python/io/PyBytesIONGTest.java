// --- file: python/io/PyBytesIONGTest.java ---
package python.io;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyBytes;
import python.lang.PyTestHarness;

public class PyBytesIONGTest extends PyTestHarness
{

  @Test
  public void testCreateEmpty()
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    assertNotNull(instance);
    assertFalse(instance.closed());
    assertTrue(instance.readable());
    assertTrue(instance.writable());
    assertTrue(instance.seekable());
  }

  @Test
  public void testWriteAndGetvalue()
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    int written = instance.write(context.bytesFromHex("68656c6c6f"));

    assertEquals(written, 5);
    PyBytes value = instance.getvalue();
    assertEquals(value.decode("utf-8", null).toString(), "hello");
  }

  @Test
  public void testCreateWithInitialBuffer()
  {
    PyBytesIO instance = IO.using(context).bytesIO(context.bytesFromHex("616263"));

    PyBytes value = instance.getvalue();
    assertEquals(value.decode("utf-8", null).toString(), "abc");
  }

  @Test
  public void testReadAll()
  {
    PyBytesIO instance = IO.using(context).bytesIO(context.bytesFromHex("616263646566"));

    PyBytes read = instance.read();

    assertEquals(read.decode("utf-8", null).toString(), "abcdef");
  }

  @Test
  public void testReadWithSize()
  {
    PyBytesIO instance = IO.using(context).bytesIO(context.bytesFromHex("616263646566"));

    PyBytes read = instance.read(3);

    assertEquals(read.decode("utf-8", null).toString(), "abc");
  }

  @Test
  public void testSeekAndTell()
  {
    PyBytesIO instance = IO.using(context).bytesIO(context.bytesFromHex("616263646566"));

    instance.seek(2);

    assertEquals(instance.tell(), 2L);
    assertEquals(instance.read().decode("utf-8", null).toString(), "cdef");
  }

  @Test
  public void testClose()
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    instance.close();

    assertTrue(instance.closed());
  }
}
