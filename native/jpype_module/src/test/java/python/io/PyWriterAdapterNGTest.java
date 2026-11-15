// --- file: python/io/PyWriterAdapterNGTest.java ---
package python.io;

import static org.testng.Assert.*;
import java.io.IOException;
import java.io.Writer;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyWriterAdapterNGTest extends PyTestHarness
{

  @Test
  public void testWriterWriteBulk() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO();

    Writer out = instance.asWriter();
    out.write("hello");
    out.flush();

    assertEquals(instance.getvalue().toString(), "hello");
  }

  @Test
  public void testWriterWriteSingleChars() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO();

    Writer out = instance.asWriter();
    out.write('a');
    out.write('b');
    out.write('c');
    out.flush();

    assertEquals(instance.getvalue().toString(), "abc");
  }

  @Test
  public void testWriterNonAsciiRoundTrip() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO();

    Writer out = instance.asWriter();
    out.write("café été");
    out.flush();

    assertEquals(instance.getvalue().toString(), "café été");
  }

  @Test
  public void testWriterFlushAndClose() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO();

    Writer out = instance.asWriter();
    out.write('x');
    out.flush();
    out.close();

    assertTrue(instance.closed());
  }

  @Test
  public void testRoundTripThroughBothAdapters() throws IOException
  {
    PyStringIO source = IO.using(context).stringIO("test");
    PyStringIO dest = IO.using(context).stringIO();

    java.io.Reader in = source.asReader();
    Writer out = dest.asWriter();
    char[] buf = new char[4096];
    int n;
    while ((n = in.read(buf)) != -1)
      out.write(buf, 0, n);
    out.flush();

    assertEquals(dest.getvalue().toString(), "test");
  }
}
