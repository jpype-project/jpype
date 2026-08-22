// --- file: python/datetime/PyDateNGTest.java ---
package python.datetime;

import java.time.LocalDate;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyDateNGTest extends PyTestHarness
{

  @Test
  public void testAccessors()
  {
    PyDate d = DateTime.using(context).date(2024, 1, 2);

    assertEquals(d.year(), 2024);
    assertEquals(d.month(), 1);
    assertEquals(d.day(), 2);
  }

  @Test
  public void testWeekdayAndIsoWeekday()
  {
    // 2024-01-02 is a Tuesday.
    PyDate d = DateTime.using(context).date(2024, 1, 2);

    assertEquals(d.weekday(), 1);
    assertEquals(d.isoWeekday(), 2);
  }

  @Test
  public void testToOrdinal()
  {
    PyDate d = DateTime.using(context).date(2024, 1, 2);

    assertEquals(d.toOrdinal(), 738887L);
  }

  @Test
  public void testToday()
  {
    PyDate d = DateTime.using(context).today();

    assertNotNull(d);
    assertTrue(d.year() >= 2024);
  }

  @Test
  public void testToLocalDate()
  {
    PyDate d = DateTime.using(context).date(2024, 1, 2);

    assertEquals(d.toLocalDate(), LocalDate.of(2024, 1, 2));
  }

  @Test
  public void testDateFromLocalDate()
  {
    PyDate d = DateTime.using(context).dateFromLocalDate(LocalDate.of(2020, 6, 15));

    assertEquals(d.year(), 2020);
    assertEquals(d.month(), 6);
    assertEquals(d.day(), 15);
  }

  @Test
  public void testCompareTo()
  {
    DateTime dt = DateTime.using(context);
    PyDate earlier = dt.date(2024, 1, 1);
    PyDate later = dt.date(2024, 1, 2);
    PyDate same = dt.date(2024, 1, 1);

    assertTrue(earlier.compareTo(later) < 0);
    assertTrue(later.compareTo(earlier) > 0);
    assertEquals(earlier.compareTo(same), 0);
  }

  @Test
  public void testEquals()
  {
    DateTime dt = DateTime.using(context);
    PyDate a = dt.date(2024, 1, 1);
    PyDate b = dt.date(2024, 1, 1);

    assertEquals(a, b);
  }

}
