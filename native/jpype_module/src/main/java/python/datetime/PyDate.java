// --- file: python/datetime/PyDate.java ---
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

import java.time.LocalDate;
import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code datetime.date} — a naive,
 * calendar (proleptic Gregorian) date with no time-of-day component.
 *
 * <p>
 * Create instances via {@link DateTime#using(python.lang.PyBuiltIn)
 * DateTime.using(context)}'s {@code date(...)}/{@code today()} factory
 * methods, not directly. Instances are immutable, matching Python's own
 * {@code date}.
 *
 * <p>
 * Comparison follows Python's rich-comparison semantics: comparing a
 * {@code PyDate} against a {@code PyDateTime} (or any other non-{@code date}
 * type) raises a bridged {@code TypeError} at runtime, exactly as it would
 * in Python — this is not caught or normalized here.
 */
public interface PyDate extends PyObject, Comparable<PyDate>
{

  /**
   * @return the proleptic Gregorian year, e.g. {@code 2024}.
   */
  int year();

  /**
   * @return the month, {@code 1}-{@code 12}.
   */
  int month();

  /**
   * @return the day of the month, {@code 1}-{@code 31} (range depends on
   * {@link #month()}/{@link #year()}).
   */
  int day();

  /**
   * @return the day of the week as an integer, {@code 0} (Monday) through
   * {@code 6} (Sunday), matching Python's {@code date.weekday()}.
   */
  int weekday();

  /**
   * @return the day of the week as an integer, {@code 1} (Monday) through
   * {@code 7} (Sunday), matching Python's {@code date.isoweekday()}.
   */
  int isoWeekday();

  /**
   * @return the proleptic Gregorian ordinal of this date, where January 1
   * of year 1 has ordinal 1, matching Python's {@code date.toordinal()}.
   */
  long toOrdinal();

  /**
   * Promotes this value to the standard Java {@link LocalDate} equivalent.
   * Computed entirely on the Java side from {@link #year()}/
   * {@link #month()}/{@link #day()} — no further round trip into Python.
   *
   * @return a {@link LocalDate} equal to this date.
   */
  default LocalDate toLocalDate()
  {
    return LocalDate.of(year(), month(), day());
  }

  /**
   * Compares this date to another following Python's rich-comparison
   * semantics.
   *
   * @param other the date to compare against.
   * @return a negative integer, zero, or a positive integer as this date is
   * less than, equal to, or greater than {@code other}.
   * @throws RuntimeException (bridged {@code TypeError}) if {@code other}
   * is not comparable to this date, e.g. a {@link PyDateTime} compared
   * against a bare {@code PyDate}.
   */
  @Override
  int compareTo(PyDate other);

}
