// --- file: python/lang/PyIterNGTest.java ---
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
import java.util.NoSuchElementException;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Exercises {@link PyIter#next()}'s default method body directly. None of
 * the interfaces that extend {@link PyIter} in this codebase (PyRange,
 * PyZip, PyFilter, PyEnumerate, PyGenerator) override {@code next()}, so any
 * {@link PyIter} obtained via {@link PyBuiltIn#iter(Object)} reaches this
 * default implementation.
 */
public class PyIterNGTest extends PyTestHarness
{

  @Test
  public void testNextReturnsElementsInOrder()
  {
    PyIter<PyObject> it = context.iter(context.list(Arrays.asList("a", "b", "c")));

    assertEquals(it.next().toString(), "a");
    assertEquals(it.next().toString(), "b");
    assertEquals(it.next().toString(), "c");
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testNextThrowsWhenExhausted()
  {
    PyIter<PyObject> it = context.iter(context.list(Arrays.asList("only")));

    it.next();
    it.next();
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testNextOnEmptyIterableThrowsImmediately()
  {
    PyIter<PyObject> it = context.iter(context.list());

    it.next();
  }

  @Test
  public void testNextWithDefaultReturnsProvidedValueWhenExhausted()
  {
    PyIter<PyObject> it = context.iter(context.list(Arrays.asList("a")));
    PyObject sentinel = context.str("sentinel");

    assertEquals(it.next().toString(), "a");
    assertEquals(it.next(sentinel), sentinel);
  }
}
