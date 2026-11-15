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
import python.lang.PyBytes;
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

  @Test
  public void testInputStreamZeroLengthReadReturnsZeroImmediately() throws IOException
  {
    PyBytesIO real = IO.using(context).bytesIO(context.bytesFromHex("0001"));
    InputStream in = new PyIOInputStream(real, 4);

    assertEquals(in.read(new byte[0], 0, 0), 0);
  }

  @Test
  public void testInputStreamBulkReadPastEofReturnsMinusOne() throws IOException
  {
    PyBytesIO real = IO.using(context).bytesIO(context.bytesFromHex("0001"));
    InputStream in = new PyIOInputStream(real, 4);

    // Drain the 2 real bytes first via the small-buffer path.
    assertEquals(in.read(), 0);
    assertEquals(in.read(), 1);

    // A request >= the buffer size on an already-exhausted stream takes
    // the bulk-bypass branch straight to a 0-length chunk -> -1.
    byte[] buf = new byte[8];
    assertEquals(in.read(buf, 0, 8), -1);
  }

  @Test
  public void testOutputStreamLargeWriteOfSubRangeCopiesInsteadOfAliasing() throws IOException
  {
    // A bulk-bypass write of a sub-range (not the whole backing array) must
    // take the Arrays.copyOfRange() branch, not alias the caller's array -
    // testLargeWriteBypassesBuffer only ever writes the full array (off==0,
    // len==b.length), which never exercises this branch.
    PyBytesIO real = IO.using(context).bytesIO();
    OutputStream out = new PyIOOutputStream(real, 4);

    byte[] data = new byte[]
    {
      9, 1, 2, 3, 4, 5, 9
    };
    out.write(data, 1, 5);

    PyBytes written = real.getvalue();
    assertEquals(written.size(), 5);
    for (int i = 0; i < 5; i++)
      assertEquals(written.builtin().asLong(written.get(i)), i + 1);
  }

  @Test
  public void testOutputStreamLargeWriteOfLeadingSubRangeCopiesInsteadOfAliasing() throws IOException
  {
    // off == 0 but len < b.length - the other way the alias short-circuit
    // (off == 0 && len == b.length) can be false, not covered by the
    // off != 0 case above.
    PyBytesIO real = IO.using(context).bytesIO();
    OutputStream out = new PyIOOutputStream(real, 4);

    byte[] data = new byte[]
    {
      1, 2, 3, 4, 5, 9, 9
    };
    out.write(data, 0, 5);

    PyBytes written = real.getvalue();
    assertEquals(written.size(), 5);
    for (int i = 0; i < 5; i++)
      assertEquals(written.builtin().asLong(written.get(i)), i + 1);
  }

  @Test
  public void testInputStreamBulkReadServesFromNonEmptyBufferWithoutRefilling() throws IOException
  {
    // pos < limit (buffer already has unconsumed data) must skip straight
    // to the bottom arraycopy path - every other bulk-read test here starts
    // from a freshly empty buffer (pos == limit), which only exercises the
    // pos >= limit branch of the guard.
    PyBytesIO real = IO.using(context).bytesIO(context.bytesFromHex("00010203"));
    InputStream in = new PyIOInputStream(real, 4);

    assertEquals(in.read(), 0); // fills the 4-byte buffer, consumes 1

    byte[] buf = new byte[2];
    int n = in.read(buf, 0, 2);

    assertEquals(n, 2);
    assertEquals(buf, new byte[]
    {
      1, 2
    });
  }

  @Test
  public void testOutputStreamZeroLengthWriteIsNoOp() throws IOException
  {
    PyBytesIO real = IO.using(context).bytesIO();
    OutputStream out = new PyIOOutputStream(real, 4);

    out.write(new byte[0], 0, 0);
    out.flush();

    assertEquals(real.getvalue().size(), 0);
  }

  @Test
  public void testOutputStreamSmallWriteOverflowingBufferFlushesFirst() throws IOException
  {
    PyBytesIO real = IO.using(context).bytesIO();
    OutputStream out = new PyIOOutputStream(real, 4);

    // 3 bytes leaves room for 1 more before the buffer (size 4) is full;
    // a second 3-byte write can't fit (3 + 3 > 4), forcing flushBuffer()
    // before it gets appended - exercises that overflow-triggered flush
    // rather than the len >= buffer.length bulk-bypass branch.
    out.write(new byte[]
    {
      1, 2, 3
    }, 0, 3);
    out.write(new byte[]
    {
      4, 5, 6
    }, 0, 3);
    out.flush();

    assertEquals(real.getvalue().size(), 6);
  }

  @Test
  public void testReaderZeroLengthReadReturnsZeroImmediately() throws IOException
  {
    PyStringIO real = IO.using(context).stringIO("ab");
    Reader in = new PyIOReader(real, 4);

    assertEquals(in.read(new char[0], 0, 0), 0);
  }

  @Test
  public void testReaderLargeReadBypassesBuffer() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    String content = "0123456789";
    PyStringIO real = IO.using(context).stringIO(content);
    PyTextIOBase spy = countingSpy(PyTextIOBase.class, real, calls);

    Reader in = new PyIOReader(spy, 4);
    char[] buf = new char[content.length()];
    int n = in.read(buf, 0, buf.length);

    assertEquals(n, content.length());
    assertEquals(new String(buf), content);
    assertEquals(calls.get(), 1, "a request >= the buffer size should bypass it entirely");
  }

  @Test
  public void testReaderBulkReadPastEofReturnsMinusOne() throws IOException
  {
    PyStringIO real = IO.using(context).stringIO("ab");
    Reader in = new PyIOReader(real, 4);

    assertEquals(in.read(), 'a');
    assertEquals(in.read(), 'b');

    char[] buf = new char[8];
    assertEquals(in.read(buf, 0, 8), -1);
  }

  @Test
  public void testWriterZeroLengthWriteIsNoOp() throws IOException
  {
    PyStringIO real = IO.using(context).stringIO();
    Writer out = new PyIOWriter(real, 4);

    out.write(new char[0], 0, 0);
    out.flush();

    assertEquals(real.getvalue().toString(), "");
  }

  @Test
  public void testWriterLargeWriteBypassesBuffer() throws IOException
  {
    AtomicInteger calls = new AtomicInteger();
    PyStringIO real = IO.using(context).stringIO();
    PyTextIOBase spy = countingSpy(PyTextIOBase.class, real, calls);

    Writer out = new PyIOWriter(spy, 4);
    out.write("0123456789");

    assertEquals(real.getvalue().toString(), "0123456789");
    assertEquals(calls.get(), 1, "a request >= the buffer size should bypass it entirely");
  }
}
