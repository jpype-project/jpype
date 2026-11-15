// --- file: python/lang/PyIterableNGTest.java ---
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

import java.util.Arrays;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises {@link PyIterable#iter()}'s default method body.
 *
 * <p>
 * Neither {@link PyList} nor {@link PyTuple} override {@code iter()} itself
 * (they only override the Java {@code iterator()} method, and that override
 * delegates back to {@code this.iter()}), so calling {@code iter()} directly
 * on either concrete type reaches {@link PyIterable}'s default
 * implementation.
 */
public class PyIterableNGTest extends PyTestHarness
{

  @Test
  public void testIterOnListReturnsWorkingIterator()
  {
    PyIterable<PyObject> list = context.list(Arrays.asList("a", "b", "c"));

    PyIter<PyObject> it = list.iter();

    assertNotNull(it);
    assertEquals(it.next().toString(), "a");
    assertEquals(it.next().toString(), "b");
    assertEquals(it.next().toString(), "c");
  }

  @Test
  public void testIterOnTupleReturnsWorkingIterator()
  {
    PyIterable<PyObject> tuple = context.tuple("x", "y");

    PyIter<PyObject> it = tuple.iter();

    assertNotNull(it);
    assertEquals(it.next().toString(), "x");
    assertEquals(it.next().toString(), "y");
  }

  @Test
  public void testIterOnEmptyListHasNoElements()
  {
    PyIterable<PyObject> list = context.list();

    PyIter<PyObject> it = list.iter();

    assertNotNull(it);
    try
    {
      it.next();
      fail("Expected NoSuchElementException");
    } catch (java.util.NoSuchElementException ex)
    {
      // expected
    }
  }
}
