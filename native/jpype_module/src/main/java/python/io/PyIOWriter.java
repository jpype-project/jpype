// --- file: python/io/PyIOWriter.java ---
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

import java.io.Writer;

/**
 * Adapts a {@link PyTextIOBase} to {@link java.io.Writer}, backing
 * {@link PyTextIOBase#asWriter()}.
 *
 * Writes accumulate in an internal buffer (default 8KB chars) and are
 * flushed to a single bulk {@link PyTextIOBase#write(CharSequence)} call
 * when the buffer fills, on {@link #flush()}, or on {@link #close()} — so
 * single-char {@code write(int)} calls (via {@link Writer}'s default
 * {@code write(int)}, which delegates to {@code write(char[], int, int)})
 * and small bulk requests no longer cost one bridge round trip each. A
 * pending write must be flushed (explicitly, or via {@code close()})
 * before its characters are visible to the underlying Python stream. A
 * request at least as large as the buffer flushes what's pending and is
 * written directly.
 */
final class PyIOWriter extends Writer
{

  static final int DEFAULT_BUFFER_SIZE = 8192;

  private final PyTextIOBase stream;
  private final char[] buffer;
  private int count;

  PyIOWriter(PyTextIOBase stream)
  {
    this(stream, DEFAULT_BUFFER_SIZE);
  }

  PyIOWriter(PyTextIOBase stream, int bufferSize)
  {
    this.stream = stream;
    this.buffer = new char[bufferSize];
  }

  private void flushBuffer()
  {
    if (count == 0)
      return;
    stream.write(new String(buffer, 0, count));
    count = 0;
  }

  @Override
  public void write(char[] cbuf, int off, int len)
  {
    if (len == 0)
      return;
    if (len >= buffer.length)
    {
      flushBuffer();
      stream.write(new String(cbuf, off, len));
      return;
    }
    if (count + len > buffer.length)
      flushBuffer();
    System.arraycopy(cbuf, off, buffer, count, len);
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
