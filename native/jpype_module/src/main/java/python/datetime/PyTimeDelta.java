// --- file: python/datetime/PyTimeDelta.java ---
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
package python.datetime;

import java.time.Duration;
import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code datetime.timedelta} — a
 * fixed span of time, unlike the calendar-aware {@link PyDate}/
 * {@link PyDateTime} split.
 *
 * <p>
 * Create instances via {@link DateTime#using(python.lang.PyBuiltIn)
 * DateTime.using(context)}'s {@code timeDelta(...)} factory methods, not
 * directly. Instances are immutable, matching Python's own {@code timedelta}.
 *
 * <p>
 * Python normalizes every {@code timedelta} so that {@link #seconds()} is in
 * {@code [0, 86400)} and {@link #microseconds()} is in {@code [0, 1000000)},
 * with the overall sign carried entirely by {@link #days()} (which may be
 * negative) — e.g. {@code timedelta(seconds=-1)} normalizes to
 * {@code days=-1, seconds=86399, microseconds=0}. This interface reports the
 * same normalized fields Python does; it does not re-derive a "natural sign"
 * split.
 */
public interface PyTimeDelta extends PyObject, Comparable<PyTimeDelta>
{

  /**
   * @return the number of whole days, positive or negative.
   */
  int days();

  /**
   * @return the normalized seconds component, {@code 0}-{@code 86399}.
   */
  int seconds();

  /**
   * @return the normalized microseconds component, {@code 0}-{@code 999999}.
   */
  int microseconds();

  /**
   * @return the total span represented by this value, in seconds, matching
   * Python's {@code timedelta.total_seconds()}.
   */
  double totalSeconds();

  /**
   * Promotes this value to the standard Java {@link Duration} equivalent.
   * Computed entirely on the Java side from {@link #days()}/
   * {@link #seconds()}/{@link #microseconds()} — no further round trip
   * into Python.
   *
   * @return a {@link Duration} equal to this span.
   */
  default Duration toDuration()
  {
    return Duration.ofDays(days()).plusSeconds(seconds()).plusNanos(microseconds() * 1000L);
  }

  /**
   * Compares this span to another following Python's rich-comparison
   * semantics.
   *
   * @param other the span to compare against.
   * @return a negative integer, zero, or a positive integer as this span is
   * shorter than, equal to, or longer than {@code other}.
   */
  @Override
  int compareTo(PyTimeDelta other);

}
