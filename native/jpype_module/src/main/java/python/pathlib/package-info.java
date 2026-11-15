// --- file: python/pathlib/package-info.java ---
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
/**
 * Java front-end for Python's {@code pathlib} module, following the same
 * conventions as {@link python.lang}, {@link python.io},
 * {@link python.collections}, and {@link python.datetime}.
 *
 * Start from {@link Pathlib#using(python.lang.PyBuiltIn)
 * Pathlib.using(context)} to construct instances of the concrete type in
 * this package: {@link Pathlib#path(String, String...) path(...)} for a
 * {@code pathlib.Path}. The returned {@link PyPath} is a normal Java
 * interface backed by the real Python object, so its methods behave
 * exactly as they would in Python.
 *
 * <p>
 * This first cut covers {@code PurePath}'s pure string/segment surface
 * ({@link PyPath#name()}, {@link PyPath#parent()}, {@link PyPath#suffix()},
 * {@link PyPath#join(String...)}, ...) plus a handful of cheap filesystem
 * predicates that are commonly needed alongside path navigation
 * ({@link PyPath#exists()}, {@link PyPath#isFile()},
 * {@link PyPath#isDirectory()}, {@link PyPath#isSymlink()}). It does not
 * yet cover the rest of {@code Path}'s I/O surface ({@code open}/
 * {@code read_text}/{@code mkdir}/{@code unlink}/...). A
 * {@code Path.open()} bridge into {@code python.io}'s stream types is
 * a natural candidate for a future extension.
 *
 * <p>
 * {@link PyPath} also offers promotion default methods to the standard
 * Java equivalents — {@link PyPath#toNioPath()} and
 * {@link PyPath#toFile()} — computed from {@link PyPath#toString()}, the
 * same conversion JPype's forward-bridge {@code SupportsPath} customizer
 * already uses for any {@code os.PathLike} (see {@code
 * jpype/protocol.py}). This plan's {@link PyPath} instead gives Java code
 * a typed front-end object for a {@code Path} value received *from*
 * Python, which the forward-bridge customizer does not provide.
 */
package python.pathlib;
