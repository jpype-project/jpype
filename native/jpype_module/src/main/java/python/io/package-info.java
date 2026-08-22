// --- file: python/io/package-info.java ---
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
 * Java front-end for Python's {@code io} module, following the same
 * conventions as {@link python.lang}.
 *
 * Start from {@link IO#using(python.lang.PyBuiltIn) IO.using(context)} to
 * construct instances of the concrete types in this package, e.g.
 * {@link IO#bytesIO() IO.using(context).bytesIO()} for an in-memory
 * {@code io.BytesIO}, or {@link IO#stringIO() stringIO()} for
 * {@code io.StringIO}. Each returned object is a normal Java interface
 * ({@link PyBytesIO}, {@link PyStringIO}, ...) backed by the real Python
 * object, so its methods behave exactly as they would in Python.
 *
 * Binary streams can be promoted to standard {@code java.io} types where
 * that's more convenient: {@link PyBufferedIOBase#asInputStream()} and
 * {@link PyBufferedIOBase#asOutputStream()} wrap a Python binary stream as
 * a {@link java.io.InputStream}/{@link java.io.OutputStream}, so it can be
 * handed to any Java API that expects one.
 */
