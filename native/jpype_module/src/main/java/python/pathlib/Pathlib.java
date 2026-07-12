// --- file: python/pathlib/Pathlib.java ---
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
package python.pathlib;

import java.io.File;
import python.lang.PyBuiltIn;

/**
 * Entry point for constructing {@code python.pathlib} objects, following
 * the same "mini-backend factory interface" convention as
 * {@code python.io.IO}, {@code python.collections.PyCollections}, and
 * {@code python.datetime.DateTime}.
 *
 * Call {@link #using(PyBuiltIn)} with the interpreter's {@code PyBuiltIn}
 * to get a {@code Pathlib} instance, then use its factory methods to
 * create {@code python.pathlib} objects:
 *
 * <pre>{@code
 * Pathlib pathlib = Pathlib.using(context);
 * PyPath p = pathlib.path("/a/b", "c.txt");
 * }</pre>
 *
 * <p>
 * As with {@code DateTime}'s {@code java.time} convenience factories, each
 * method that accepts a non-{@code String} convenience type is given a
 * distinct name from {@link #path(String, String...)} rather than an
 * overload of it — JPype's proxy dispatch for {@code WrapperService}-backed
 * interfaces routes purely by method name, not by Java overload signature.
 * See {@code plan/SPI_tutorial.md}.
 */
public interface Pathlib
{

  static Pathlib using(PyBuiltIn context)
  {
    return context.getBackend(Pathlib.class);
  }

  /**
   * Creates a new {@code pathlib.Path}, matching Python's
   * {@code Path(first, *more)}.
   *
   * @param first the first path segment.
   * @param more additional path segments, joined onto {@code first} in
   * order.
   * @return a new {@link PyPath} instance.
   */
  PyPath path(String first, String... more);

  /**
   * Creates a new {@code pathlib.Path} equal to the given {@link File}.
   *
   * @param file the value to convert.
   * @return a new {@link PyPath} instance.
   */
  default PyPath pathFromFile(File file)
  {
    return path(file.getPath());
  }

  /**
   * Creates a new {@code pathlib.Path} equal to the given
   * {@link java.nio.file.Path}.
   *
   * @param nioPath the value to convert.
   * @return a new {@link PyPath} instance.
   */
  default PyPath pathFromNioPath(java.nio.file.Path nioPath)
  {
    return path(nioPath.toString());
  }

}
