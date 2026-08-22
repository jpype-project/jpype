// --- file: python/datetime/DateTime.java ---
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
import java.time.Instant;
import java.time.LocalDate;
import java.time.LocalDateTime;
import python.lang.PyBuiltIn;

/**
 * Entry point for constructing {@code python.datetime} objects, following
 * the same "mini-backend factory interface" convention as
 * {@code python.io.IO} and {@code python.collections.PyCollections}.
 *
 * Call {@link #using(PyBuiltIn)} with the interpreter's {@code PyBuiltIn}
 * to get a {@code DateTime} instance, then use its factory methods to
 * create {@code python.datetime} objects:
 *
 * <pre>{@code
 * DateTime datetime = DateTime.using(context);
 * PyDate d = datetime.date(2024, 1, 2);
 * PyDateTime dt = datetime.now();
 * PyTimeDelta span = datetime.timeDeltaFromDuration(Duration.ofHours(3));
 * }</pre>
 *
 * <p>
 * Each factory method that accepts a {@code java.time} convenience type
 * (e.g. {@link #dateFromLocalDate(LocalDate)}) is deliberately given a
 * distinct name from its primitive-argument counterpart rather than an
 * overload of it ({@code date(LocalDate)} sharing the name {@code date}
 * with {@code date(int, int, int)}). JPype's proxy dispatch for
 * {@code WrapperService}-backed interfaces routes purely by method name,
 * not by Java overload signature — a {@code default} method sharing a
 * registered method's name is intercepted and routed to that method's
 * {@code .pyspi} callable with whatever arguments were actually passed,
 * without ever running the {@code default} method's own body. Overloading
 * {@code date(int, int, int)}/{@code date(LocalDate)} under one name would
 * silently forward a raw {@code LocalDate} object to a Python callable
 * that expects three integers.
 */
public interface DateTime
{

  static DateTime using(PyBuiltIn context)
  {
    return context.getBackend(DateTime.class);
  }

  /**
   * Creates a new {@code datetime.date}.
   *
   * @param year the proleptic Gregorian year.
   * @param month the month, {@code 1}-{@code 12}.
   * @param day the day of the month.
   * @return a new {@link PyDate} instance.
   */
  PyDate date(int year, int month, int day);

  /**
   * Creates a new {@code datetime.date} equal to the given {@link LocalDate}.
   *
   * @param date the value to convert.
   * @return a new {@link PyDate} instance.
   */
  default PyDate dateFromLocalDate(LocalDate date)
  {
    return date(date.getYear(), date.getMonthValue(), date.getDayOfMonth());
  }

  /**
   * @return a new {@code datetime.date} for the current day, in the host's
   * local timezone, matching Python's {@code date.today()}.
   */
  PyDate today();

  /**
   * Creates a new, naive {@code datetime.datetime}.
   *
   * @param year the proleptic Gregorian year.
   * @param month the month, {@code 1}-{@code 12}.
   * @param day the day of the month.
   * @param hour the hour, {@code 0}-{@code 23}.
   * @param minute the minute, {@code 0}-{@code 59}.
   * @param second the second, {@code 0}-{@code 59}.
   * @param microsecond the microsecond, {@code 0}-{@code 999999}.
   * @return a new, naive {@link PyDateTime} instance.
   */
  PyDateTime dateTime(int year, int month, int day, int hour, int minute, int second, int microsecond);

  /**
   * Creates a new, naive {@code datetime.datetime} equal to the given
   * {@link LocalDateTime}.
   *
   * @param dateTime the value to convert.
   * @return a new, naive {@link PyDateTime} instance.
   */
  default PyDateTime dateTimeFromLocalDateTime(LocalDateTime dateTime)
  {
    return dateTime(dateTime.getYear(), dateTime.getMonthValue(), dateTime.getDayOfMonth(),
            dateTime.getHour(), dateTime.getMinute(), dateTime.getSecond(),
            dateTime.getNano() / 1000);
  }

  /**
   * Creates a new, UTC-aware {@code datetime.datetime} whose instant is
   * {@code epochSeconds} seconds since the Unix epoch, matching
   * {@code datetime.datetime.fromtimestamp(epochSeconds, tz=timezone.utc)}.
   *
   * @param epochSeconds seconds since the Unix epoch (may carry a
   * fractional part down to microsecond precision).
   * @return a new, timezone-aware {@link PyDateTime} instance
   * ({@link PyDateTime#isAware()} is {@code true}).
   */
  PyDateTime dateTimeFromEpochSeconds(double epochSeconds);

  /**
   * Creates a new, UTC-aware {@code datetime.datetime} equal to the given
   * {@link Instant}.
   *
   * @param instant the value to convert.
   * @return a new, timezone-aware {@link PyDateTime} instance
   * ({@link PyDateTime#isAware()} is {@code true}).
   */
  default PyDateTime dateTimeFromInstant(Instant instant)
  {
    return dateTimeFromEpochSeconds(instant.getEpochSecond() + instant.getNano() / 1e9);
  }

  /**
   * @return a new, naive {@code datetime.datetime} for the current moment,
   * in the host's local timezone, matching Python's {@code datetime.now()}.
   */
  PyDateTime now();

  /**
   * Creates a new {@code datetime.timedelta}.
   *
   * @param days whole days, positive or negative.
   * @param seconds whole seconds; need not already be normalized.
   * @param microseconds microseconds; need not already be normalized.
   * @return a new {@link PyTimeDelta} instance, with fields normalized the
   * same way Python's {@code timedelta} constructor normalizes them.
   */
  PyTimeDelta timeDelta(int days, int seconds, int microseconds);

  /**
   * Creates a new {@code datetime.timedelta} equal to the given
   * {@link Duration}.
   *
   * @param duration the value to convert.
   * @return a new {@link PyTimeDelta} instance.
   */
  default PyTimeDelta timeDeltaFromDuration(Duration duration)
  {
    // Duration's (seconds, nanos) pair already carries the sign in seconds
    // with nanos held to a canonical non-negative [0, 1e9) range; Python's
    // timedelta constructor normalizes an out-of-range seconds/microseconds
    // pair the same way its days/seconds/microseconds trio would be, so no
    // day/second split needs to happen on the Java side first.
    return timeDelta(0, (int) duration.getSeconds(), duration.getNano() / 1000);
  }

}
