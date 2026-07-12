// --- file: python/io/PyIOBase.java ---
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

import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code io.IOBase}, the abstract base
 * of every stream-like object in the {@code io} module.
 *
 * Not instantiable directly; concrete classes ({@link PyBytesIO},
 * {@link PyStringIO}, ...) implement this indirectly through
 * {@link PyRawIOBase}, {@link PyBufferedIOBase}, or {@link PyTextIOBase}.
 */
public interface PyIOBase extends PyObject
{

  /**
   * Closes the stream. No further I/O operations are permitted after this
   * call. Calling {@code close()} more than once is a no-op, matching
   * Python's {@code io.IOBase.close()}.
   */
  void close();

  /**
   * @return {@code true} if the stream has been closed.
   */
  boolean closed();

  /**
   * Flushes any pending writes.
   */
  void flush();

  /**
   * @return {@code true} if this stream can be read from.
   */
  boolean readable();

  /**
   * @return {@code true} if this stream can be written to.
   */
  boolean writable();

  /**
   * @return {@code true} if this stream supports random access
   * ({@link #seek}/{@link #tell}).
   */
  boolean seekable();

  /**
   * Changes the stream position.
   *
   * @param offset the offset, interpreted relative to {@code whence}.
   * @param whence one of {@code 0} (start, the default), {@code 1}
   * (current position), or {@code 2} (end), matching
   * {@code os.SEEK_SET}/{@code SEEK_CUR}/{@code SEEK_END}.
   * @return the new absolute position.
   */
  long seek(long offset, int whence);

  /**
   * Equivalent to {@code seek(offset, 0)}.
   *
   * @param offset the absolute offset to seek to.
   * @return the new absolute position.
   */
  default long seek(long offset)
  {
    return seek(offset, 0);
  }

  /**
   * @return the current stream position.
   */
  long tell();

}
