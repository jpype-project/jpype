// --- file: python/io/PyStringIO.java ---
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

import python.lang.PyString;

/**
 * Java front-end interface for the concrete Python {@code io.StringIO} type
 * — an in-memory text stream.
 *
 * Create instances via {@code context.builtin().stringIO()} /
 * {@code stringIO(CharSequence)}, not directly.
 */
public interface PyStringIO extends PyTextIOBase
{

  /**
   * @return the entire contents of the buffer as a new {@link PyString},
   * regardless of the current stream position.
   */
  PyString getvalue();

}
