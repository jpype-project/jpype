// --- file: python/lang/PySliceNGTest.java ---
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

import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PySliceNGTest extends PyTestHarness
{

  @Test
  public void testStartStop()
  {
    PySlice slice = context.slice(1, 5);
    assertEquals(slice.getStart(), Integer.valueOf(1));
    assertEquals(slice.getStop(), Integer.valueOf(5));
    assertNull(slice.getStep());
  }

  @Test
  public void testStartStopStep()
  {
    PySlice slice = context.slice(0, 10, 2);
    assertEquals(slice.getStart(), Integer.valueOf(0));
    assertEquals(slice.getStop(), Integer.valueOf(10));
    assertEquals(slice.getStep(), Integer.valueOf(2));
  }

  @Test
  public void testOpenStart()
  {
    PySlice slice = context.slice(null, 5, null);
    assertNull(slice.getStart());
    assertEquals(slice.getStop(), Integer.valueOf(5));
  }

  @Test
  public void testIndices()
  {
    PySlice slice = context.slice(0, 5, 2);
    PyTuple indices = slice.indices(10);
    assertEquals(indices.size(), 3);
    assertEquals(indices.get(0).toString(), "0");
    assertEquals(indices.get(1).toString(), "5");
    assertEquals(indices.get(2).toString(), "2");
  }

  @Test
  public void testIsValid()
  {
    PySlice slice = context.slice(0, 5, 1);
    assertTrue(slice.isValid());
  }

  @Test
  public void testIndicesNormalizesOpenEndedSlice()
  {
    PySlice slice = context.slice(null, null, 2);
    PyTuple indices = slice.indices(10);
    assertEquals(indices.get(0).toString(), "0");
    assertEquals(indices.get(1).toString(), "10");
    assertEquals(indices.get(2).toString(), "2");
  }

}
