// --- file: python/io/PyIOStreamAdapterNGTest.java ---
package python.io;

import static org.testng.Assert.*;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyIOStreamAdapterNGTest extends PyTestHarness
{

  @Test
  public void testInputStreamReadsWholeContent() throws IOException
  {
    PyBytesIO instance = IO.using(context).bytesIO(context.bytesFromHex("68656c6c6f"));

    InputStream in = instance.asInputStream();
    byte[] buf = new byte[5];
    int n = in.read(buf);

    assertEquals(n, 5);
    assertEquals(new String(buf, "UTF-8"), "hello");
    assertEquals(in.read(), -1);
  }

  @Test
  public void testInputStreamSingleByteReads() throws IOException
  {
    PyBytesIO instance = IO.using(context).bytesIO(context.bytesFromHex("616263"));

    InputStream in = instance.asInputStream();

    assertEquals(in.read(), 'a');
    assertEquals(in.read(), 'b');
    assertEquals(in.read(), 'c');
    assertEquals(in.read(), -1);
  }

  @Test
  public void testInputStreamCloseClosesStream() throws IOException
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    instance.asInputStream().close();

    assertTrue(instance.closed());
  }

  @Test
  public void testOutputStreamWriteBulk() throws IOException
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    OutputStream out = instance.asOutputStream();
    out.write("hello".getBytes("UTF-8"));
    out.flush();

    assertEquals(instance.getvalue().decode("utf-8", null).toString(), "hello");
  }

  @Test
  public void testOutputStreamWriteSingleBytes() throws IOException
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    OutputStream out = instance.asOutputStream();
    out.write('a');
    out.write('b');
    out.write('c');
    out.flush();

    assertEquals(instance.getvalue().decode("utf-8", null).toString(), "abc");
  }

  @Test
  public void testOutputStreamFlushAndClose() throws IOException
  {
    PyBytesIO instance = IO.using(context).bytesIO();

    OutputStream out = instance.asOutputStream();
    out.write('x');
    out.flush();
    out.close();

    assertTrue(instance.closed());
  }

  @Test
  public void testRoundTripThroughBothAdapters() throws IOException
  {
    PyBytesIO source = IO.using(context).bytesIO(context.bytesFromHex("74657374"));
    PyBytesIO dest = IO.using(context).bytesIO();

    InputStream in = source.asInputStream();
    OutputStream out = dest.asOutputStream();
    byte[] buf = new byte[4096];
    int n;
    while ((n = in.read(buf)) != -1)
      out.write(buf, 0, n);
    out.flush();

    assertEquals(dest.getvalue().decode("utf-8", null).toString(), "test");
  }
}
