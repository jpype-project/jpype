// --- file: python/io/PyIOOutputStream.java ---
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

import java.util.Arrays;
import python.lang.PyBytes;

/**
 * Adapts a {@link PyBufferedIOBase} to {@link java.io.OutputStream}, backing
 * {@link PyBufferedIOBase#asOutputStream()}.
 *
 * Writes accumulate in an internal buffer (default 8KB) and are flushed to
 * a single bulk {@link PyBufferedIOBase#write(python.lang.PyBuffer)} call
 * when the buffer fills, on {@link #flush()}, or on {@link #close()} — so
 * single-byte {@code write(int)} calls and small {@code write(byte[], int,
 * int)} requests no longer cost one bridge round trip each. A pending
 * write must be flushed (explicitly, or via {@code close()}) before its
 * bytes are visible to the underlying Python stream. A request at least as
 * large as the buffer flushes what's pending and is written directly.
 */
final class PyIOOutputStream extends java.io.OutputStream
{

  static final int DEFAULT_BUFFER_SIZE = 8192;

  private final PyBufferedIOBase stream;
  private final byte[] buffer;
  private int count;

  PyIOOutputStream(PyBufferedIOBase stream)
  {
    this(stream, DEFAULT_BUFFER_SIZE);
  }

  PyIOOutputStream(PyBufferedIOBase stream, int bufferSize)
  {
    this.stream = stream;
    this.buffer = new byte[bufferSize];
  }

  private void flushBuffer()
  {
    if (count == 0)
      return;
    PyBytes bytes = stream.builtin().bytes(Arrays.copyOf(buffer, count));
    stream.write(bytes);
    count = 0;
  }

  @Override
  public void write(int b)
  {
    if (count >= buffer.length)
      flushBuffer();
    buffer[count++] = (byte) b;
  }

  @Override
  public void write(byte[] b, int off, int len)
  {
    if (len == 0)
      return;
    if (len >= buffer.length)
    {
      flushBuffer();
      byte[] chunk = (off == 0 && len == b.length) ? b : Arrays.copyOfRange(b, off, off + len);
      stream.write(stream.builtin().bytes(chunk));
      return;
    }
    if (count + len > buffer.length)
      flushBuffer();
    System.arraycopy(b, off, buffer, count, len);
    count += len;
  }

  @Override
  public void flush()
  {
    flushBuffer();
    stream.flush();
  }

  @Override
  public void close()
  {
    flushBuffer();
    stream.close();
  }

}
