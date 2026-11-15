// --- file: python/io/IO.java ---
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

import python.lang.PyBuffer;
import python.lang.PyBuiltIn;

/**
 * Entry point for constructing {@code python.io} objects.
 *
 * Call {@link #using(PyBuiltIn)} with the interpreter's {@code PyBuiltIn}
 * (e.g. {@code obj.builtin()} for a live {@code PyObject}, or the
 * {@code PyBuiltIn}/{@code Script} already in hand) to get an {@code IO}
 * instance, then use its factory methods to create {@code python.io}
 * objects:
 *
 * <pre>{@code
 * IO io = IO.using(context);
 * PyBytesIO buf = io.bytesIO();
 * PyStringIO text = io.stringIO("hello");
 * }</pre>
 */
public interface IO
{

  static IO using(PyBuiltIn context)
  {
    return context.getBackend(IO.class);
  }

  /**
   * Creates a new, empty {@code io.BytesIO}.
   *
   * @return a new {@link PyBytesIO} instance.
   */
  PyBytesIO bytesIO();

  /**
   * Creates a new {@code io.BytesIO} pre-populated with the contents of
   * {@code initial}.
   *
   * @param initial the initial contents of the stream.
   * @return a new {@link PyBytesIO} instance.
   */
  PyBytesIO bytesIO(PyBuffer initial);

  /**
   * Creates a new, empty {@code io.StringIO}.
   *
   * @return a new {@link PyStringIO} instance.
   */
  PyStringIO stringIO();

  /**
   * Creates a new {@code io.StringIO} pre-populated with {@code initial}.
   *
   * @param initial the initial contents of the stream.
   * @return a new {@link PyStringIO} instance.
   */
  PyStringIO stringIO(CharSequence initial);

  /**
   * Opens {@code path} as a raw, unbuffered binary stream, equivalent to
   * Python's {@code open(path, mode, buffering=0)}.
   *
   * @param path the file path to open.
   * @param mode the mode string, e.g. {@code "rb"} or {@code "wb"}.
   * @return a new {@link PyFileIO} instance.
   */
  PyFileIO fileIO(CharSequence path, CharSequence mode);

}
