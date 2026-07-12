// --- file: python/pathlib/PyPath.java ---
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
import java.nio.file.Paths;
import python.lang.PyList;
import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code pathlib.Path} — in practice
 * always one of its concrete platform subclasses, {@code PosixPath} or
 * {@code WindowsPath}, since {@code pathlib.Path()} always dispatches to
 * one of the two rather than ever constructing a bare {@code Path}
 * instance. Both subclasses report {@code "pathlib"} as {@code __module__},
 * so both are registered against this same interface — see
 * {@code pathlib.PosixPath.pyspi} / {@code pathlib.WindowsPath.pyspi}.
 *
 * <p>
 * Create instances via {@link Pathlib#using(python.lang.PyBuiltIn)
 * Pathlib.using(context)}'s {@code path(...)} factory method, not
 * directly. Instances are immutable, matching Python's own {@code Path}.
 *
 * <p>
 * This first cut covers {@code PurePath}'s pure string/segment operations
 * (no OS access) plus a handful of cheap, commonly-paired filesystem
 * predicates ({@link #exists()}, {@link #isFile()}, {@link #isDirectory()},
 * {@link #isSymlink()}). It deliberately does not cover the rest of
 * {@code Path}'s I/O surface ({@code open}/{@code read_text}/{@code mkdir}/
 * {@code unlink}/...) — see {@code plan/Pathlib.md} for the scope
 * discussion; a {@code Path.open()} bridge to {@code python.io}'s stream
 * types is left for a follow-up rather than folded in here.
 */
public interface PyPath extends PyObject, Comparable<PyPath>
{

  /**
   * @return the final path component, excluding the drive/root, e.g.
   * {@code "setup.py"} for {@code PosixPath("/a/b/setup.py")}. Empty for a
   * root path.
   */
  String name();

  /**
   * @return {@link #name()} with its final {@link #suffix()} (if any)
   * stripped, e.g. {@code "setup"} for {@code "setup.py"}.
   */
  String stem();

  /**
   * @return the final component's file extension, including the leading
   * dot, e.g. {@code ".py"}, or an empty string if {@link #name()} has no
   * extension.
   */
  String suffix();

  /**
   * @return every extension in {@link #name()}, including leading dots,
   * e.g. {@code [".tar", ".gz"]} for {@code "archive.tar.gz"}.
   */
  PyList suffixes();

  /**
   * @return every component of this path, in order, including the
   * drive/root as the first element if present, matching Python's
   * {@code Path.parts}.
   */
  PyList parts();

  /**
   * @return the logical parent of this path, matching Python's
   * {@code Path.parent}. A root path is its own parent.
   */
  PyPath parent();

  /**
   * @return the concatenation of {@link #drive()} and {@link #root()},
   * e.g. {@code "/"} on POSIX, or {@code "C:\\"} on Windows.
   */
  String anchor();

  /**
   * @return the drive letter/UNC share, or an empty string on platforms
   * without one (e.g. POSIX).
   */
  String drive();

  /**
   * @return the root, e.g. {@code "/"} on POSIX if this path is absolute,
   * or an empty string for a relative path.
   */
  String root();

  /**
   * @return {@code true} if this path has both a root and (on Windows) a
   * drive, matching Python's {@code Path.is_absolute()}.
   */
  boolean isAbsolute();

  /**
   * @return this path's string form with forward slashes as separators,
   * matching Python's {@code Path.as_posix()} — notably, this differs from
   * {@link #toString()} (which uses the native separator) on Windows.
   */
  String asPosix();

  /**
   * Joins this path with each of {@code segments} in turn, matching
   * Python's {@code Path.joinpath(*segments)} (and equivalently, chained
   * {@code /} operators).
   *
   * @param segments the path segments to append.
   * @return a new path with {@code segments} appended.
   */
  PyPath join(String... segments);

  /**
   * @param name the replacement final component.
   * @return a new path equal to this one but with {@link #name()} replaced
   * by {@code name}.
   */
  PyPath withName(String name);

  /**
   * @param suffix the replacement extension, including the leading dot (or
   * an empty string to remove the extension).
   * @return a new path equal to this one but with {@link #suffix()}
   * replaced by {@code suffix}.
   */
  PyPath withSuffix(String suffix);

  /**
   * @param pattern a glob-style pattern, matching Python's
   * {@code Path.match(pattern)}.
   * @return {@code true} if this path matches {@code pattern}.
   */
  boolean matches(String pattern);

  /**
   * @param other the candidate ancestor path.
   * @return {@code true} if this path is {@code other} or one of its
   * descendants, matching Python's {@code Path.is_relative_to(other)}.
   */
  boolean isRelativeTo(PyPath other);

  /**
   * @param other the path to compute this path relative to.
   * @return a new path such that {@code other.join(*result.parts())}
   * (roughly) reconstructs this path, matching Python's
   * {@code Path.relative_to(other)}.
   * @throws RuntimeException (bridged {@code ValueError}) if this path is
   * not a descendant of {@code other}.
   */
  PyPath relativeTo(PyPath other);

  /**
   * @return {@code true} if a filesystem entry currently exists at this
   * path, matching Python's {@code Path.exists()}.
   */
  boolean exists();

  /**
   * @return {@code true} if this path currently points at a regular file,
   * matching Python's {@code Path.is_file()}.
   */
  boolean isFile();

  /**
   * @return {@code true} if this path currently points at a directory,
   * matching Python's {@code Path.is_dir()}.
   */
  boolean isDirectory();

  /**
   * @return {@code true} if this path currently points at a symbolic
   * link, matching Python's {@code Path.is_symlink()}.
   */
  boolean isSymlink();

  /**
   * Promotes this value to the standard Java {@link java.nio.file.Path}
   * equivalent, via {@link #toString()} (this path's native-separator
   * string form) — the same conversion JPype's forward-bridge
   * {@code SupportsPath} customizer already uses for any
   * {@code os.PathLike}, see {@code jpype/protocol.py}.
   *
   * @return a {@link java.nio.file.Path} equal to this path.
   */
  default java.nio.file.Path toNioPath()
  {
    return Paths.get(toString());
  }

  /**
   * Promotes this value to the standard Java {@link File} equivalent, via
   * {@link #toString()}.
   *
   * @return a {@link File} equal to this path.
   */
  default File toFile()
  {
    return new File(toString());
  }

  /**
   * Compares this path to another following Python's rich-comparison
   * semantics.
   *
   * @param other the path to compare against.
   * @return a negative integer, zero, or a positive integer as this path
   * is less than, equal to, or greater than {@code other}.
   * @throws RuntimeException (bridged {@code TypeError}) if {@code other}
   * is not comparable to this path, e.g. a {@code PosixPath} compared
   * against a {@code WindowsPath}.
   */
  @Override
  int compareTo(PyPath other);

}
