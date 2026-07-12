// --- file: python/io/PyIOBufferingBenchmark.java ---
package python.io;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Reader;
import java.io.Writer;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

/**
 * Not run as part of the normal suite (excluded by surefire's default
 * {@code *Test.java} pattern, unlike the {@code *NGTest.java} suite). Run
 * explicitly with {@code mvn -o test -Dtest=PyIOBufferingBenchmark
 * -Dpython.executable=python3.10} to see real wall-clock numbers for the
 * buffered rewrite (plan/IO.md section C) against the pre-buffering
 * baseline (one bridge round trip per single-byte call) on the real
 * reverse bridge, not a Java-only simulation.
 */
public class PyIOBufferingBenchmark extends PyTestHarness
{

  private static final int N = 20_000;

  @Test
  public void benchSingleByteReads() throws IOException
  {
    // Both sides go through the identical fill()/extraction path in
    // PyIOInputStream - the only difference is buffer size 1 (a fill, and
    // therefore a stream.read(n) bridge call, on every single-byte read;
    // this is exactly what the pre-buffering adapter did) vs the real
    // default. This isolates the win this rewrite actually claims (fewer
    // stream.read(n) round trips), rather than conflating it with the
    // separately-scoped, still-open per-element PyBytes->byte[] extraction
    // cost (see plan/IO.md's "Open follow-up") which both sides pay equally.
    PyBytesIO a = IO.using(context).bytesIO(context.bytes(new byte[N]));
    long t0 = System.nanoTime();
    InputStream unbufferedStream = new PyIOInputStream(a, 1);
    for (int i = 0; i < N; i++)
      unbufferedStream.read();
    long unbuffered = System.nanoTime() - t0;

    PyBytesIO b = IO.using(context).bytesIO(context.bytes(new byte[N]));
    long t1 = System.nanoTime();
    InputStream in = b.asInputStream();
    for (int i = 0; i < N; i++)
      in.read();
    long buffered = System.nanoTime() - t1;

    report("single-byte read()", unbuffered, buffered);
  }

  @Test
  public void benchSingleByteWrites() throws IOException
  {
    PyBytesIO raw = IO.using(context).bytesIO();
    long t0 = System.nanoTime();
    for (int i = 0; i < N; i++)
      raw.write(raw.builtin().bytes(new byte[]
      {
        (byte) i
      }));
    long unbuffered = System.nanoTime() - t0;

    PyBytesIO instance = IO.using(context).bytesIO();
    long t1 = System.nanoTime();
    OutputStream out = instance.asOutputStream();
    for (int i = 0; i < N; i++)
      out.write(i);
    out.flush();
    long buffered = System.nanoTime() - t1;

    report("single-byte write(int)", unbuffered, buffered);
  }

  @Test
  public void benchSingleCharReads() throws IOException
  {
    PyStringIO a = IO.using(context).stringIO(new String(new char[N]).replace((char) 0, 'x'));
    long t0 = System.nanoTime();
    Reader unbufferedReader = new PyIOReader(a, 1);
    for (int i = 0; i < N; i++)
      unbufferedReader.read();
    long unbuffered = System.nanoTime() - t0;

    PyStringIO b = IO.using(context).stringIO(new String(new char[N]).replace((char) 0, 'x'));
    long t1 = System.nanoTime();
    Reader in = b.asReader();
    for (int i = 0; i < N; i++)
      in.read();
    long buffered = System.nanoTime() - t1;

    report("single-char read()", unbuffered, buffered);
  }

  @Test
  public void benchSingleCharWrites() throws IOException
  {
    PyStringIO raw = IO.using(context).stringIO();
    long t0 = System.nanoTime();
    for (int i = 0; i < N; i++)
      raw.write(String.valueOf((char) ('a' + (i % 26))));
    long unbuffered = System.nanoTime() - t0;

    PyStringIO instance = IO.using(context).stringIO();
    long t1 = System.nanoTime();
    Writer out = instance.asWriter();
    for (int i = 0; i < N; i++)
      out.write('a' + (i % 26));
    out.flush();
    long buffered = System.nanoTime() - t1;

    report("single-char write(int)", unbuffered, buffered);
  }

  private static void report(String label, long unbufferedNanos, long bufferedNanos)
  {
    double unbufferedMs = unbufferedNanos / 1_000_000.0;
    double bufferedMs = bufferedNanos / 1_000_000.0;
    double speedup = unbufferedMs / bufferedMs;
    System.out.printf(
            "[bench] %s x%d: unbuffered=%.2fms buffered=%.2fms speedup=%.1fx%n",
            label, N, unbufferedMs, bufferedMs, speedup);
  }

}
