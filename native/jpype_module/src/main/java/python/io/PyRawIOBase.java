// --- file: python/io/PyRawIOBase.java ---
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

import python.lang.PyByteArray;
import python.lang.PyBytes;
import python.lang.PyBuffer;

/**
 * Java front-end interface for Python's {@code io.RawIOBase} — unbuffered
 * binary streams talking directly to the OS, e.g. {@link PyFileIO}.
 */
public interface PyRawIOBase extends PyIOBase
{

  /**
   * Reads and returns up to {@code size} bytes with at most one low-level
   * call. If {@code size} is negative or omitted, equivalent to
   * {@link #readall()}, matching {@code io.RawIOBase.read}.
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
   * Reads and returns all remaining bytes, matching
   * {@code io.RawIOBase.readall}.
   *
   * @return the bytes read; empty (not {@code null}) at EOF.
   */
  PyBytes readall();

  /**
   * Reads bytes into {@code buffer}, matching {@code io.RawIOBase.readinto}.
   *
   * @param buffer the mutable buffer to read into.
   * @return the number of bytes read, or {@code 0} at EOF.
   */
  int readinto(PyByteArray buffer);

  /**
   * Writes the contents of {@code buffer} to the stream.
   *
   * @param buffer the bytes to write.
   * @return the number of bytes written.
   */
  int write(PyBuffer buffer);

}
