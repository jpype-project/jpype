// --- file: python/datetime/package-info.java ---
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
 * Java front-end for Python's {@code datetime} module, following the same
 * conventions as {@link python.lang}, {@link python.io}, and
 * {@link python.collections}.
 *
 * Start from {@link DateTime#using(python.lang.PyBuiltIn)
 * DateTime.using(context)} to construct instances of the concrete types
 * in this package: {@link DateTime#date(int, int, int) date(...)} /
 * {@link DateTime#today() today()} for a {@code datetime.date},
 * {@link DateTime#dateTime(int, int, int, int, int, int, int)
 * dateTime(...)} / {@link DateTime#now() now()} for a
 * {@code datetime.datetime}, and
 * {@link DateTime#timeDelta(int, int, int) timeDelta(...)} for a
 * {@code datetime.timedelta}. Each returned object is a normal Java
 * interface ({@link PyDate}, {@link PyDateTime}, {@link PyTimeDelta})
 * backed by the real Python object, so its methods behave exactly as they
 * would in Python.
 *
 * Python's {@code datetime.datetime} is itself a subclass of
 * {@code datetime.date}, so {@link PyDateTime} extends {@link PyDate} here
 * too, inheriting its calendar accessors. {@code datetime.timedelta}
 * represents a fixed span rather than a calendar point, so
 * {@link PyTimeDelta} stands on its own.
 *
 * Every type here also offers a promotion default method to the
 * corresponding {@code java.time} type — {@link PyDate#toLocalDate()},
 * {@link PyDateTime#toLocalDateTime()}, {@link PyDateTime#toInstant()}
 * (aware instances only), and {@link PyTimeDelta#toDuration()} — computed
 * entirely on the Java side from already-fetched accessor values, with no
 * further round trip into Python.
 */
package python.datetime;
