// --- file: python/io/PyBufferedIOBase.java ---
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
import java.io.OutputStream;
import python.lang.PyBytes;
import python.lang.PyBuffer;

/**
 * Java front-end interface for Python's {@code io.BufferedIOBase} — binary
 * streams with (Python-side) buffering, e.g. {@link PyBytesIO}.
 */
public interface PyBufferedIOBase extends PyIOBase
{

  /**
   * Reads and returns up to {@code size} bytes. If {@code size} is negative
   * or omitted, reads until EOF, matching {@code io.BufferedIOBase.read}.
   *
   * @param size the maximum number of bytes to read, or a negative value to
   * read until EOF.
   * @return the bytes read; empty (not {@code null}) at EOF.
   */
  PyBytes read(int size);

  /**
   * Equivalent to {@code read(-1)}: reads until EOF.
   *
   * @return the bytes read; empty (not {@code null}) at EOF.
   */
  PyBytes read();

  /**
   * Writes the contents of {@code buffer} to the stream.
   *
   * @param buffer the bytes to write.
   * @return the number of bytes written.
   */
  int write(PyBuffer buffer);

  /**
   * Wraps this stream as a standard {@link java.io.InputStream}, so it can be
   * passed to Java APIs that expect one. The returned stream buffers reads
   * internally (default 8KB), refilling with a single {@link #read(int)}
   * call, so single-byte {@code read()} calls and small bulk requests don't
   * each cost their own round trip into Python; a request at least as large
   * as the buffer bypasses it.
   *
   * @return an {@code InputStream} view of this stream.
   */
  default InputStream asInputStream()
  {
    return new PyIOInputStream(this);
  }

  /**
   * Wraps this stream as a standard {@link java.io.OutputStream}, so it can
   * be passed to Java APIs that expect one. The returned stream buffers
   * writes internally (default 8KB), flushing to a single
   * {@link #write(PyBuffer)} call when the buffer fills, on {@code flush()},
   * or on {@code close()} — a pending write is not visible on the underlying
   * stream until then. A request at least as large as the buffer bypasses it.
   *
   * @return an {@code OutputStream} view of this stream.
   */
  default OutputStream asOutputStream()
  {
    return new PyIOOutputStream(this);
  }

}
