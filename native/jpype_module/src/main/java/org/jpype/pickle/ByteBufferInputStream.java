/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.pickle;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.LinkedList;

/**
 * A specialized {@link InputStream} implementation backed by multiple
 * {@link ByteBuffer} objects.
 *
 * <p>
 * This class allows for efficient reading of data from multiple byte buffers,
 * treating them as a single continuous stream. It provides methods to add byte
 * arrays to the stream and overrides standard {@link InputStream} methods for
 * reading data.</p>
 */
public class ByteBufferInputStream extends InputStream
{

  private LinkedList<ByteBuffer> buffers = new LinkedList<>();

  /**
   * Adds a byte array to the stream by wrapping it in a {@link ByteBuffer}.
   *
   * <p>
   * The byte array is wrapped directly without copying, so changes to the array
   * after it is added will affect the data in the stream.</p>
   *
   * @param bytes The byte array to add to the stream.
   */
  public void put(byte[] bytes)
  {
    ByteBuffer buffer = ByteBuffer.wrap(bytes);
    buffers.add(buffer);
  }

  /**
   * Reads the next byte of data from the stream.
   *
   * <p>
   * If the stream is empty or the end of the stream is reached, this method
   * returns -1.</p>
   *
   * @return The next byte of data as an integer in the range 0 to 255, or -1 if
   * the end of the stream is reached.
   * @throws IOException If an I/O error occurs.
   */
  @Override
  public int read() throws IOException
  {
    if (buffers.isEmpty())
      return -1;

    ByteBuffer b = buffers.getFirst();
    while (b.remaining() == 0)
    {
      buffers.removeFirst();
      if (buffers.isEmpty())
        return -1; // EOF
      b = buffers.getFirst();
    }
    return b.get() & 0xFF; // Mask with 0xFF to convert signed byte to int (range 0-255)
  }

  /**
   * Reads up to {@code buffer.length} bytes of data from the stream into the
   * specified byte array.
   *
   * <p>
   * This method is a convenience wrapper for
   * {@link #read(byte[], int, int)}.</p>
   *
   * @param buffer The byte array into which data is read.
   * @return The number of bytes read, or -1 if the end of the stream is
   * reached.
   * @throws IOException If an I/O error occurs.
   */
  @Override
  public int read(byte[] buffer) throws IOException
  {
    return read(buffer, 0, buffer.length);
  }

  /**
   * Reads up to {@code len} bytes of data from the stream into the specified
   * byte array starting at {@code offset}.
   *
   * <p>
   * This method reads data from the stream into the provided buffer, starting
   * at the specified offset and reading up to the specified length.</p>
   *
   * @param buffer The byte array into which data is read.
   * @param offset The starting offset in the byte array.
   * @param len The maximum number of bytes to read.
   * @return The number of bytes read, or -1 if the end of the stream is
   * reached.
   * @throws IOException If an I/O error occurs.
   * @throws NullPointerException If {@code buffer} is null.
   * @throws IndexOutOfBoundsException If {@code offset} or {@code len} are
   * invalid.
   */
  @Override
  public int read(byte[] buffer, int offset, int len) throws IOException
  {
    if (buffer == null)
      throw new NullPointerException("Buffer cannot be null");
    if (offset < 0 || len < 0 || len > buffer.length - offset)
      throw new IndexOutOfBoundsException("Invalid offset/length parameters");
    if (len == 0)
      return 0;

    int total = 0;
    while (len > 0 && !buffers.isEmpty())
    {
      ByteBuffer b = buffers.getFirst();
      int remaining = b.remaining();
      if (remaining == 0)
      {
        buffers.removeFirst();
        continue;
      }

      int toRead = Math.min(len, remaining);
      b.get(buffer, offset, toRead);
      total += toRead;
      len -= toRead;
      offset += toRead;
    }
    return (total == 0) ? -1 : total;
  }

  /**
   * Closes the stream and releases all resources.
   *
   * <p>
   * This method clears all {@link ByteBuffer} objects from the stream.</p>
   *
   * @throws IOException If an I/O error occurs.
   */
  @Override
  public void close() throws IOException
  {
    buffers.clear();
  }
}
