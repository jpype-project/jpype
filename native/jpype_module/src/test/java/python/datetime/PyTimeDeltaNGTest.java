// --- file: python/datetime/PyTimeDeltaNGTest.java ---
package python.datetime;

import java.time.Duration;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyTimeDeltaNGTest extends PyTestHarness
{

  @Test
  public void testAccessors()
  {
    PyTimeDelta td = DateTime.using(context).timeDelta(1, 2, 3);

    assertEquals(td.days(), 1);
    assertEquals(td.seconds(), 2);
    assertEquals(td.microseconds(), 3);
  }

  @Test
  public void testNormalization()
  {
    // Matches CPython: timedelta(seconds=-1) normalizes to
    // days=-1, seconds=86399, microseconds=0.
    PyTimeDelta td = DateTime.using(context).timeDelta(0, -1, 0);

    assertEquals(td.days(), -1);
    assertEquals(td.seconds(), 86399);
    assertEquals(td.microseconds(), 0);
  }

  @Test
  public void testTotalSeconds()
  {
    PyTimeDelta td = DateTime.using(context).timeDelta(1, 2, 3);

    assertEquals(td.totalSeconds(), 86402.000003, 0.0000001);
  }

  @Test
  public void testToDuration()
  {
    PyTimeDelta td = DateTime.using(context).timeDelta(1, 2, 3);

    assertEquals(td.toDuration(), Duration.ofDays(1).plusSeconds(2).plusNanos(3000));
  }

  @Test
  public void testTimeDeltaFromDuration()
  {
    Duration duration = Duration.ofHours(3).plusMillis(500);
    PyTimeDelta td = DateTime.using(context).timeDeltaFromDuration(duration);

    assertEquals(td.totalSeconds(), 3 * 3600 + 0.5, 0.0000001);
  }

  @Test
  public void testCompareTo()
  {
    DateTime factory = DateTime.using(context);
    PyTimeDelta shorter = factory.timeDelta(0, 1, 0);
    PyTimeDelta longer = factory.timeDelta(0, 2, 0);

    assertTrue(shorter.compareTo(longer) < 0);
    assertTrue(longer.compareTo(shorter) > 0);
  }

}
