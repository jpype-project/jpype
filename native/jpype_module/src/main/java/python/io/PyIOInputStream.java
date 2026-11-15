// --- file: python/io/PyIOInputStream.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package python.io;

import java.io.InputStream;
import python.lang.PyBytes;

/**
 * Adapts a {@link PyBufferedIOBase} to {@link java.io.InputStream}, backing
 * {@link PyBufferedIOBase#asInputStream()}.
 *
 * Lets a {@code python.io} object stand in wherever Java code expects a
 * real {@code InputStream}. Reads are served out of an internal buffer
 * (default 8KB) refilled with a single bulk {@link PyBufferedIOBase#read(int)}
 * call, so single-byte {@code read()} calls and small {@code read(byte[],
 * int, int)} requests no longer cost one bridge round trip each; a request
 * at least as large as the buffer bypasses it and reads straight into the
 * caller's array.
 */
final class PyIOInputStream extends InputStream
{

  static final int DEFAULT_BUFFER_SIZE = 8192;

  private final PyBufferedIOBase stream;
  private final byte[] buffer;
  private int pos;
  private int limit;

  PyIOInputStream(PyBufferedIOBase stream)
  {
    this(stream, DEFAULT_BUFFER_SIZE);
  }

  PyIOInputStream(PyBufferedIOBase stream, int bufferSize)
  {
    this.stream = stream;
    this.buffer = new byte[bufferSize];
  }

  /**
   * Refills {@code buffer} from a single bulk {@code read()} call.
   *
   * @return the number of bytes filled; {@code 0} at EOF.
   */
  private int fill()
  {
    PyBytes chunk = stream.read(buffer.length);
    int n = chunk.size();
    for (int i = 0; i < n; i++)
      buffer[i] = (byte) (chunk.builtin().asLong(chunk.get(i)) & 0xFF);
    pos = 0;
    limit = n;
    return n;
  }

  @Override
  public int read()
  {
    if (pos >= limit && fill() == 0)
      return -1;
    return buffer[pos++] & 0xFF;
  }

  @Override
  public int read(byte[] b, int off, int len)
  {
    if (len == 0)
      return 0;
    if (pos >= limit)
    {
      // Buffer is empty: a request at least as large as the buffer gets no
      // benefit from buffering, so read straight into the caller's array.
      if (len >= buffer.length)
      {
        PyBytes chunk = stream.read(len);
        int n = chunk.size();
        if (n == 0)
          return -1;
        for (int i = 0; i < n; i++)
          b[off + i] = (byte) (chunk.builtin().asLong(chunk.get(i)) & 0xFF);
        return n;
      }
      if (fill() == 0)
        return -1;
    }
    int n = Math.min(len, limit - pos);
    System.arraycopy(buffer, pos, b, off, n);
    pos += n;
    return n;
  }

  @Override
  public void close()
  {
    stream.close();
  }

}
