// --- file: python/io/PyBytesIO.java ---
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

import python.lang.PyBytes;

/**
 * Java front-end interface for the concrete Python {@code io.BytesIO} type
 * — an in-memory binary stream.
 *
 * Create instances via {@code context.builtin().bytesIO()} /
 * {@code bytesIO(PyBuffer)}, not directly.
 */
public interface PyBytesIO extends PyBufferedIOBase
{

  /**
   * @return the entire contents of the buffer as a new {@link PyBytes},
   * regardless of the current stream position.
   */
  PyBytes getvalue();

}
