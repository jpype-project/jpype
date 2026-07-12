// --- file: python/io/PyIOBufferingNGTest.java ---
package python.io;

import static org.testng.Assert.*;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.io.Writer;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.concurrent.atomic.AtomicInteger;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

/**
 * Regression guard for the buffered rewrite of the {@code python.io}
 * adapters (plan/IO.md section C): confirms N small Java-side calls collapse
 * into O(N / bufferSize) calls into Python, not O(N) — correctness alone
 * (covered by {@link PyIOStreamAdapterNGTest}/{@link PyReaderAdapterNGTest}/
 * {@link PyWriterAdapterNGTest}) wouldn't catch a regression back to 1:1
 * forwarding.
 */
public class PyIOBufferingNGTest extends PyTestHarness
{

  /**
   * Wraps {@code real} in a dynamic proxy implementing {@code iface} that
   * counts calls to {@code read}/{@code write} while forwarding every call
   * (including the counted ones) straight through to {@code real}.
   */
  @SuppressWarnings("unchecked")
  private static <T> T countingSpy(Class<T> iface, T real, AtomicInteger counter)
  {
    InvocationHandler handler = (proxy, method, args) ->
    {
      String name = method.getName();
      if (name.equals("read") || name.equals("write"))
        counter.incrementAndGet();
      try
      {
        return method.invoke(real, args);
      } catch (java.lang.reflect.InvocationTargetException e)
      {
        throw e.getCause();
      }
    };
    return (T) Proxy.newProxyInstance(iface.getClassLoader(), new Class<?>[]
    {
      iface
    }, handler);
  }

  @Test
  public void testInputStreamBuffersSmallReads() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    PyBytesIO real = IO.using(context).bytesIO(
            context.bytesFromHex("000102030405060708090a0b0c0d0e0f10111213"));
    PyBufferedIOBase spy = countingSpy(PyBufferedIOBase.class, real, calls);

    InputStream in = new PyIOInputStream(spy, 4);
    // Read exactly the known content length, stopping short of the EOF
    // probe (one extra fill to confirm -1) so the count isn't muddied by
    // that unavoidable last round trip.
    for (int i = 0; i < 20; i++)
      assertEquals(in.read(), i);

    assertEquals(calls.get(), 5, "20 bytes through a 4-byte buffer should take 5 fills");
  }

  @Test
  public void testOutputStreamBuffersSmallWrites() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    PyBytesIO real = IO.using(context).bytesIO();
    PyBufferedIOBase spy = countingSpy(PyBufferedIOBase.class, real, calls);

    OutputStream out = new PyIOOutputStream(spy, 4);
    for (int i = 0; i < 20; i++)
      out.write(i);
    out.flush();

    assertEquals(real.getvalue().size(), 20);
    assertEquals(calls.get(), 5, "20 bytes through a 4-byte buffer should take 5 flushes");
  }

  @Test
  public void testReaderBuffersSmallReads() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    PyStringIO real = IO.using(context).stringIO("abcdefghijklmnopqrst");
    PyTextIOBase spy = countingSpy(PyTextIOBase.class, real, calls);

    Reader in = new PyIOReader(spy, 4);
    // Read exactly the known content length, stopping short of the EOF
    // probe (one extra fill to confirm -1) so the count isn't muddied by
    // that unavoidable last round trip.
    for (int i = 0; i < 20; i++)
      assertEquals(in.read(), "abcdefghijklmnopqrst".charAt(i));

    assertEquals(calls.get(), 5, "20 chars through a 4-char buffer should take 5 fills");
  }

  @Test
  public void testWriterBuffersSmallWrites() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    PyStringIO real = IO.using(context).stringIO();
    PyTextIOBase spy = countingSpy(PyTextIOBase.class, real, calls);

    Writer out = new PyIOWriter(spy, 4);
    for (char c = 'a'; c < 'a' + 20; c++)
      out.write(c);
    out.flush();

    assertEquals(real.getvalue().toString().length(), 20);
    assertEquals(calls.get(), 5, "20 chars through a 4-char buffer should take 5 flushes");
  }

  @Test
  public void testLargeReadBypassesBuffer() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    byte[] data = new byte[100];
    for (int i = 0; i < data.length; i++)
      data[i] = (byte) i;
    PyBytesIO real = IO.using(context).bytesIO(context.bytes(data));
    PyBufferedIOBase spy = countingSpy(PyBufferedIOBase.class, real, calls);

    InputStream in = new PyIOInputStream(spy, 4);
    byte[] buf = new byte[100];
    int n = in.read(buf);

    assertEquals(n, 100);
    assertEquals(calls.get(), 1, "a request >= the buffer size should bypass it entirely");
  }

  @Test
  public void testLargeWriteBypassesBuffer() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    PyBytesIO real = IO.using(context).bytesIO();
    PyBufferedIOBase spy = countingSpy(PyBufferedIOBase.class, real, calls);

    OutputStream out = new PyIOOutputStream(spy, 4);
    byte[] data = new byte[100];
    for (int i = 0; i < data.length; i++)
      data[i] = (byte) i;
    out.write(data);

    assertEquals(real.getvalue().size(), 100);
    assertEquals(calls.get(), 1, "a request >= the buffer size should bypass it entirely");
  }
}
