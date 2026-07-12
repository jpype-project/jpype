// --- file: python/io/PyStringIONGTest.java ---
package python.io;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyString;
import python.lang.PyTestHarness;

public class PyStringIONGTest extends PyTestHarness
{

  @Test
  public void testCreateEmpty()
  {
    PyStringIO instance = IO.using(context).stringIO();

    assertNotNull(instance);
    assertFalse(instance.closed());
  }

  @Test
  public void testWriteAndGetvalue()
  {
    PyStringIO instance = IO.using(context).stringIO();

    int written = instance.write("hello");

    assertEquals(written, 5);
    PyString value = instance.getvalue();
    assertEquals(value.toString(), "hello");
  }

  @Test
  public void testCreateWithInitialString()
  {
    PyStringIO instance = IO.using(context).stringIO("abc");

    PyString value = instance.getvalue();
    assertEquals(value.toString(), "abc");
  }

  @Test
  public void testReadAll()
  {
    PyStringIO instance = IO.using(context).stringIO("abcdef");

    PyString read = instance.read();

    assertEquals(read.toString(), "abcdef");
  }

  @Test
  public void testReadWithSize()
  {
    PyStringIO instance = IO.using(context).stringIO("abcdef");

    PyString read = instance.read(3);

    assertEquals(read.toString(), "abc");
  }

  @Test
  public void testReadline()
  {
    PyStringIO instance = IO.using(context).stringIO("first\nsecond\n");

    PyString line = instance.readline();

    assertEquals(line.toString(), "first\n");
  }

  @Test
  public void testSeekAndTell()
  {
    PyStringIO instance = IO.using(context).stringIO("abcdef");

    instance.seek(2);

    assertEquals(instance.tell(), 2L);
    assertEquals(instance.read().toString(), "cdef");
  }

  @Test
  public void testClose()
  {
    PyStringIO instance = IO.using(context).stringIO();

    instance.close();

    assertTrue(instance.closed());
  }
}
