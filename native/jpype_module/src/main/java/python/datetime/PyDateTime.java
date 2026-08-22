// --- file: python/datetime/PyDateTime.java ---
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

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneOffset;

/**
 * Java front-end interface for Python's {@code datetime.datetime} — a date
 * with an attached time-of-day, either naive or timezone-aware. Python's own
 * {@code datetime} is a subclass of {@code date}, so this interface extends
 * {@link PyDate} the same way, inheriting {@link #year()}/{@link #month()}/
 * {@link #day()} and friends.
 *
 * <p>
 * Create instances via {@link DateTime#using(python.lang.PyBuiltIn)
 * DateTime.using(context)}'s {@code dateTime(...)}/{@code now()} family of
 * factory methods, not directly. Instances are immutable, matching Python's
 * own {@code datetime}.
 *
 * <p>
 * A {@code datetime} is <b>aware</b> if it carries a non-{@code None}
 * {@code tzinfo} (i.e. {@code utcoffset()} does not return {@code None});
 * check with {@link #isAware()} before relying on {@link #toInstant()},
 * which needs a real UTC offset to be meaningful. A <b>naive</b> instance
 * has no associated timezone; Python treats it as "local time" for
 * operations like {@code timestamp()}, but it cannot be unambiguously
 * promoted to an {@link Instant} without one, so {@link #toInstant()}
 * throws for naive values — use {@link #toLocalDateTime()} instead.
 */
public interface PyDateTime extends PyDate
{

  /**
   * @return the hour, {@code 0}-{@code 23}.
   */
  int hour();

  /**
   * @return the minute, {@code 0}-{@code 59}.
   */
  int minute();

  /**
   * @return the second, {@code 0}-{@code 59}.
   */
  int second();

  /**
   * @return the microsecond, {@code 0}-{@code 999999}.
   */
  int microsecond();

  /**
   * @return {@code true} if this instance carries timezone information
   * (a non-{@code None} {@code tzinfo}), {@code false} if it is naive.
   */
  boolean isAware();

  /**
   * @return this instance's UTC offset in seconds, or {@code null} if this
   * instance is naive ({@link #isAware()} is {@code false}).
   */
  Integer utcOffsetSeconds();

  /**
   * @return the POSIX timestamp (seconds since the Unix epoch) for this
   * instant, matching Python's {@code datetime.timestamp()}. For a naive
   * instance, Python (and therefore this method) assumes this value
   * represents local time on the host running the interpreter.
   */
  double timestamp();

  /**
   * Promotes this value's date and time-of-day fields to the standard
   * Java {@link LocalDateTime} equivalent, discarding any timezone
   * information. Computed entirely on the Java side from the accessor
   * methods above — no further round trip into Python.
   *
   * @return a {@link LocalDateTime} matching this instance's naive
   * calendar/clock fields.
   */
  default LocalDateTime toLocalDateTime()
  {
    return LocalDateTime.of(year(), month(), day(), hour(), minute(), second(), microsecond() * 1000);
  }

  /**
   * Promotes this value to the standard Java {@link Instant} equivalent,
   * using {@link #utcOffsetSeconds()} to interpret {@link #toLocalDateTime()}
   * as an absolute point in time.
   *
   * @return an {@link Instant} equal to this datetime.
   * @throws IllegalStateException if this instance is naive
   * ({@link #isAware()} is {@code false}) and therefore has no UTC offset
   * to promote with.
   */
  default Instant toInstant()
  {
    Integer offsetSeconds = utcOffsetSeconds();
    if (offsetSeconds == null)
    {
      throw new IllegalStateException(
              "naive datetime has no UTC offset; use toLocalDateTime() instead of toInstant()");
    }
    return toLocalDateTime().toInstant(ZoneOffset.ofTotalSeconds(offsetSeconds));
  }

}
