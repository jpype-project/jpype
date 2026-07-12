// --- file: python/io/PyIOReader.java ---
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

import java.io.Reader;
import python.lang.PyString;

/**
 * Adapts a {@link PyTextIOBase} to {@link java.io.Reader}, backing
 * {@link PyTextIOBase#asReader()}.
 *
 * Lets a {@code python.io} text object stand in wherever Java code expects a
 * real {@code Reader}. Reads are served out of an internal buffer (default
 * 8KB chars) refilled with a single bulk {@link PyTextIOBase#read(int)}
 * call, so single-char {@code read()} calls (via {@link Reader}'s default
 * {@code read()}, which delegates to {@code read(char[], int, int)}) and
 * small bulk requests no longer cost one bridge round trip each; a request
 * at least as large as the buffer bypasses it and reads straight into the
 * caller's array.
 */
final class PyIOReader extends Reader
{

  static final int DEFAULT_BUFFER_SIZE = 8192;

  private final PyTextIOBase stream;
  private final char[] buffer;
  private int pos;
  private int limit;

  PyIOReader(PyTextIOBase stream)
  {
    this(stream, DEFAULT_BUFFER_SIZE);
  }

  PyIOReader(PyTextIOBase stream, int bufferSize)
  {
    this.stream = stream;
    this.buffer = new char[bufferSize];
  }

  private int fill()
  {
    PyString chunk = stream.read(buffer.length);
    int n = chunk.length();
    chunk.toString().getChars(0, n, buffer, 0);
    pos = 0;
    limit = n;
    return n;
  }

  @Override
  public int read(char[] cbuf, int off, int len)
  {
    if (len == 0)
      return 0;
    if (pos >= limit)
    {
      if (len >= buffer.length)
      {
        PyString chunk = stream.read(len);
        int n = chunk.length();
        if (n == 0)
          return -1;
        chunk.toString().getChars(0, n, cbuf, off);
        return n;
      }
      if (fill() == 0)
        return -1;
    }
    int n = Math.min(len, limit - pos);
    System.arraycopy(buffer, pos, cbuf, off, n);
    pos += n;
    return n;
  }

  @Override
  public void close()
  {
    stream.close();
  }

}
