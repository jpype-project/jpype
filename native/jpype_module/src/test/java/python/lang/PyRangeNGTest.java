// --- file: python/lang/PyRangeNGTest.java ---
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
package python.lang;

import java.util.Iterator;
import python.exceptions.PyIndexError;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyRangeNGTest extends PyTestHarness
{

  @Test
  public void testStopOnly()
  {
    PyRange range = context.range(5);
    assertEquals(range.getStart(), 0);
    assertEquals(range.getStop(), 5);
    assertEquals(range.getStep(), 1);
    assertEquals(range.getLength(), 5);
  }

  @Test
  public void testStartStop()
  {
    PyRange range = context.range(2, 7);
    assertEquals(range.getStart(), 2);
    assertEquals(range.getStop(), 7);
    assertEquals(range.getLength(), 5);
  }

  @Test
  public void testStartStopStep()
  {
    PyRange range = context.range(0, 10, 2);
    assertEquals(range.getStep(), 2);
    assertEquals(range.getLength(), 5);
  }

  @Test
  public void testGetItem()
  {
    PyRange range = context.range(1, 5);
    assertEquals(range.getItem(0), 1);
    assertEquals(range.getItem(3), 4);
  }

  @Test(expectedExceptions = PyIndexError.class)
  public void testGetItemOutOfBounds()
  {
    PyRange range = context.range(0, 3);
    range.getItem(10);
  }

  @Test
  public void testGetSlice()
  {
    PyRange range = context.range(0, 10);
    PyRange slice = range.getSlice(2, 5);
    assertEquals(slice.getLength(), 3);
    assertEquals(slice.getItem(0), 2);
  }

  @Test
  public void testContains()
  {
    PyRange range = context.range(0, 10, 2);
    assertTrue(range.contains(4));
    assertFalse(range.contains(5));
  }

  @Test
  public void testIterator()
  {
    PyRange range = context.range(0, 3);
    Iterator<PyInt> it = range.iterator();
    int total = 0;
    while (it.hasNext())
      total += it.next().toNumber().intValue();
    assertEquals(total, 3);
  }

}
