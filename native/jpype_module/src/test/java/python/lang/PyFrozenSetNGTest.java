// --- file: python/lang/PyFrozenSetNGTest.java ---
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

public class PyFrozenSetNGTest extends PyTestHarness
{

  @Test
  public void testCreate()
  {
    PyFrozenSet set = context.frozensetFromIterable(Arrays.asList(context.str("a"), context.str("b")));
    assertNotNull(set);
    assertEquals(set.size(), 2);
  }

  @Test
  public void testFrozensetFromObject()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a", "b", "a"));
    assertEquals(set.size(), 2);
  }

  @Test
  public void testContains()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a", "b"));
    assertTrue(set.contains(context.str("a")));
    assertFalse(set.contains(context.str("z")));
  }

  @Test
  public void testCopy()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a", "b"));
    PyFrozenSet copy = set.copy();
    assertEquals(copy.size(), 2);
  }

  @Test
  public void testDifference()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a", "b", "c"));
    PyFrozenSet b = context.frozenset(Arrays.asList("b"));
    PyFrozenSet result = a.difference(b);
    assertEquals(result.size(), 2);
    assertFalse(result.contains(context.str("b")));
  }

  @Test
  public void testIntersect()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a", "b"));
    PyFrozenSet b = context.frozenset(Arrays.asList("b", "c"));
    PyFrozenSet result = a.intersection(b);
    assertEquals(result.size(), 1);
    assertTrue(result.contains(context.str("b")));
  }

  @Test
  public void testIsDisjoint()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a", "b"));
    PyFrozenSet b = context.frozenset(Arrays.asList("c", "d"));
    assertTrue(a.isDisjoint(b));
  }

  @Test
  public void testIsSubset()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a"));
    PyFrozenSet b = context.frozenset(Arrays.asList("a", "b"));
    assertTrue(a.isSubset(b));
    assertFalse(b.isSubset(a));
  }

  @Test
  public void testIsSuperset()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a", "b"));
    PyFrozenSet b = context.frozenset(Arrays.asList("a"));
    assertTrue(a.isSuperset(b));
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testPopIsUnsupported()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a"));
    set.pop();
  }

  @Test
  public void testSymmetricDifference()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a", "b"));
    PyFrozenSet b = context.frozenset(Arrays.asList("b", "c"));
    PyFrozenSet result = a.symmetricDifference(b);
    assertEquals(result.size(), 2);
    assertTrue(result.contains(context.str("a")));
    assertTrue(result.contains(context.str("c")));
  }

  @Test
  public void testUnion()
  {
    PyFrozenSet a = context.frozenset(Arrays.asList("a", "b"));
    PyFrozenSet b = context.frozenset(Arrays.asList("b", "c"));
    PyFrozenSet result = a.union(b);
    assertEquals(result.size(), 3);
  }

  @Test
  public void testIterator()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a", "b"));
    int count = 0;
    for (PyObject obj : set)
      count++;
    assertEquals(count, 2);
  }

  @Test
  public void testToArray()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a", "b"));
    Object[] array = set.toArray();
    assertEquals(array.length, 2);
  }

  @Test
  public void testToArrayTyped()
  {
    PyFrozenSet set = context.frozenset(Arrays.asList("a", "b"));
    PyObject[] result = set.toArray(new PyObject[0]);
    assertEquals(result.length, 2);

    PyObject[] bigArray = new PyObject[5];
    PyObject[] result2 = set.toArray(bigArray);
    assertSame(result2, bigArray);
    assertNull(result2[2]);
  }

}
