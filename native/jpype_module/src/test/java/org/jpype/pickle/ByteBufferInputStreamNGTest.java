// --- file: org/jpype/pickle/ByteBufferInputStreamNGTest.java ---
package org.jpype.pickle;

import java.io.IOException;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Pure Java bookkeeping - no native/bridge dependency required.
 */
public class ByteBufferInputStreamNGTest
{

  @Test
  public void testCloseDiscardsUnreadData() throws IOException
  {
    ByteBufferInputStream in = new ByteBufferInputStream();
    in.put(new byte[]
    {
      1, 2, 3
    });
    assertEquals(in.read(), 1);
    in.close();
    // close() clears the backing buffer list, so even the byte that was
    // never read is gone - the stream reads as empty/EOF afterward.
    assertEquals(in.read(), -1);
  }

  @Test
  public void testCloseIsIdempotent() throws IOException
  {
    ByteBufferInputStream in = new ByteBufferInputStream();
    in.put(new byte[]
    {
      9
    });
    in.close();
    // Calling close() a second time must not throw.
    in.close();
    assertEquals(in.read(), -1);
  }

  @Test
  public void testCloseOnEmptyStreamDoesNotThrow() throws IOException
  {
    ByteBufferInputStream in = new ByteBufferInputStream();
    in.close();
    assertEquals(in.read(), -1);
  }

  @Test
  public void testPutAfterCloseIsReadableAgain() throws IOException
  {
    // close() only clears the buffer list - it is not a terminal "stream is
    // dead forever" state per the source (no closed flag is tracked), so a
    // put() after close() should be readable exactly like a fresh stream.
    ByteBufferInputStream in = new ByteBufferInputStream();
    in.put(new byte[]
    {
      5, 6
    });
    in.close();
    in.put(new byte[]
    {
      7
    });
    assertEquals(in.read(), 7);
    assertEquals(in.read(), -1);
  }
}
