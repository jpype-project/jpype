// --- file: python/io/PyBufferedWriter.java ---
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
 * Java front-end interface for the concrete Python {@code io.BufferedWriter}
 * type — a buffered wrapper around a raw binary stream, opened for writing.
 *
 * Create instances via {@code IO.using(context).fileIO(path, mode)} followed
 * by Python-side buffering (e.g. plain {@code open(path, "wb")}), not
 * directly.
 */
public interface PyBufferedWriter extends PyBufferedIOBase
{
}
