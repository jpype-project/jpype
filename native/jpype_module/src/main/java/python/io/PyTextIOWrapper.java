// --- file: python/io/PyTextIOWrapper.java ---
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

/**
 * Java front-end interface for the concrete Python {@code io.TextIOWrapper}
 * type — a text stream layered over a buffered binary stream, e.g. what
 * {@code open(path, "r")} returns.
 *
 * Create instances via {@code IO.using(context).fileIO(path, mode)} followed
 * by Python-side text wrapping (e.g. plain {@code open(path, "r")}), not
 * directly.
 */
public interface PyTextIOWrapper extends PyTextIOBase
{

  /**
   * @return {@code true} if writes are flushed to the underlying binary
   * buffer on every newline.
   */
  boolean line_buffering();

  /**
   * @return {@code true} if writes pass straight through to the underlying
   * binary buffer without being held in this wrapper's own buffer.
   */
  boolean write_through();

}
