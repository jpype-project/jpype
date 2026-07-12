// --- file: python/datetime/PyDateTimeNGTest.java ---
package python.datetime;

import java.time.Instant;
import java.time.LocalDateTime;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyDateTimeNGTest extends PyTestHarness
{

  @Test
  public void testAccessors()
  {
    PyDateTime dt = DateTime.using(context).dateTime(2024, 1, 2, 3, 4, 5, 6789);

    assertEquals(dt.year(), 2024);
    assertEquals(dt.month(), 1);
    assertEquals(dt.day(), 2);
    assertEquals(dt.hour(), 3);
    assertEquals(dt.minute(), 4);
    assertEquals(dt.second(), 5);
    assertEquals(dt.microsecond(), 6789);
  }

  @Test
  public void testNaiveIsNotAware()
  {
    PyDateTime dt = DateTime.using(context).dateTime(2024, 1, 2, 3, 4, 5, 6789);

    assertFalse(dt.isAware());
    assertNull(dt.utcOffsetSeconds());
  }

  @Test
  public void testAwareFromEpochSeconds()
  {
    PyDateTime dt = DateTime.using(context).dateTimeFromEpochSeconds(1704164645.006789);

    assertTrue(dt.isAware());
    assertEquals((int) dt.utcOffsetSeconds(), 0);
    assertEquals(dt.year(), 2024);
    assertEquals(dt.month(), 1);
    assertEquals(dt.day(), 2);
    assertEquals(dt.hour(), 3);
    assertEquals(dt.minute(), 4);
    assertEquals(dt.second(), 5);
  }

  @Test
  public void testNow()
  {
    PyDateTime dt = DateTime.using(context).now();

    assertNotNull(dt);
    assertFalse(dt.isAware());
    assertTrue(dt.year() >= 2024);
  }

  @Test
  public void testToLocalDateTime()
  {
    PyDateTime dt = DateTime.using(context).dateTime(2024, 1, 2, 3, 4, 5, 6789);

    assertEquals(dt.toLocalDateTime(), LocalDateTime.of(2024, 1, 2, 3, 4, 5, 6789 * 1000));
  }

  @Test
  public void testDateTimeFromLocalDateTime()
  {
    LocalDateTime ldt = LocalDateTime.of(2020, 6, 15, 10, 30, 0, 123000);
    PyDateTime dt = DateTime.using(context).dateTimeFromLocalDateTime(ldt);

    assertEquals(dt.year(), 2020);
    assertEquals(dt.hour(), 10);
    assertEquals(dt.minute(), 30);
    assertEquals(dt.microsecond(), 123);
    assertFalse(dt.isAware());
  }

  @Test
  public void testToInstantForAware()
  {
    Instant instant = Instant.parse("2024-01-02T03:04:05.006789Z");
    PyDateTime dt = DateTime.using(context).dateTimeFromInstant(instant);

    assertTrue(dt.isAware());
    assertEquals(dt.toInstant(), instant);
  }

  @Test(expectedExceptions = IllegalStateException.class)
  public void testToInstantThrowsForNaive()
  {
    PyDateTime dt = DateTime.using(context).dateTime(2024, 1, 2, 3, 4, 5, 0);

    dt.toInstant();
  }

  @Test
  public void testTimestamp()
  {
    PyDateTime dt = DateTime.using(context).dateTimeFromInstant(Instant.ofEpochSecond(1704164645));

    assertEquals(dt.timestamp(), 1704164645.0, 0.001);
  }

  @Test
  public void testIsPyDate()
  {
    // datetime.datetime is a subclass of datetime.date in Python; the Java
    // interface hierarchy mirrors that, so the inherited PyDate accessors
    // must work directly on a PyDateTime instance.
    PyDateTime dt = DateTime.using(context).dateTime(2024, 1, 2, 3, 4, 5, 0);
    PyDate d = dt;

    assertEquals(d.year(), 2024);
    assertEquals(d.toOrdinal(), 738887L);
  }

  @Test
  public void testCompareTo()
  {
    DateTime factory = DateTime.using(context);
    PyDateTime earlier = factory.dateTime(2024, 1, 1, 0, 0, 0, 0);
    PyDateTime later = factory.dateTime(2024, 1, 1, 0, 0, 1, 0);

    assertTrue(earlier.compareTo(later) < 0);
    assertTrue(later.compareTo(earlier) > 0);
  }

}
