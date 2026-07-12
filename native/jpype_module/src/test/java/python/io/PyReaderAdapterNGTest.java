// --- file: python/io/PyReaderAdapterNGTest.java ---
package python.io;

import static org.testng.Assert.*;
import java.io.IOException;
import java.io.Reader;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyReaderAdapterNGTest extends PyTestHarness
{

  @Test
  public void testReaderReadsWholeContent() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO("hello");

    Reader in = instance.asReader();
    char[] buf = new char[5];
    int n = in.read(buf);

    assertEquals(n, 5);
    assertEquals(new String(buf), "hello");
    assertEquals(in.read(), -1);
  }

  @Test
  public void testReaderSingleCharReads() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO("abc");

    Reader in = instance.asReader();

    assertEquals(in.read(), 'a');
    assertEquals(in.read(), 'b');
    assertEquals(in.read(), 'c');
    assertEquals(in.read(), -1);
  }

  @Test
  public void testReaderNonAsciiRoundTrip() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO("café été");

    Reader in = instance.asReader();
    char[] buf = new char[16];
    int n = in.read(buf);

    assertEquals(new String(buf, 0, n), "café été");
  }

  @Test
  public void testReaderCloseClosesStream() throws IOException
  {
    PyStringIO instance = IO.using(context).stringIO();

    instance.asReader().close();

    assertTrue(instance.closed());
  }
}
