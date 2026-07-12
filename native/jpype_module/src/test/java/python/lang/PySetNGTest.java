// --- file: python/lang/PySetNGTest.java ---
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

public class PySetNGTest extends PyTestHarness
{

  @Test
  public void testEmpty()
  {
    PySet set = context.set();
    assertNotNull(set);
    assertTrue(set.isEmpty());
    assertEquals(set.size(), 0);
  }

  @Test
  public void testAdd()
  {
    PySet set = context.set();
    assertTrue(set.add(context.str("a")));
    assertEquals(set.size(), 1);
    assertFalse(set.add(context.str("a")));
    assertEquals(set.size(), 1);
  }

  @Test
  public void testAddAny()
  {
    PySet set = context.set();
    set.addAny(5);
    assertEquals(set.size(), 1);
  }

  @Test
  public void testClear()
  {
    PySet set = context.set(Arrays.asList("a", "b"));
    set.clear();
    assertTrue(set.isEmpty());
  }

  @Test
  public void testContains()
  {
    PySet set = context.set(Arrays.asList("a", "b"));
    assertTrue(set.contains(context.str("a")));
    assertFalse(set.contains(context.str("z")));
  }

  @Test
  public void testCopy()
  {
    PySet set = context.set(Arrays.asList("a", "b"));
    PySet copy = set.copy();
    assertEquals(copy.size(), 2);
    copy.add(context.str("c"));
    assertEquals(set.size(), 2);
  }

  @Test
  public void testDifference()
  {
    PySet a = context.set(Arrays.asList("a", "b", "c"));
    PySet b = context.set(Arrays.asList("b"));
    PySet result = a.difference(b);
    assertEquals(result.size(), 2);
    assertTrue(result.contains(context.str("a")));
    assertFalse(result.contains(context.str("b")));
  }

  @Test
  public void testDifferenceUpdate()
  {
    PySet a = context.set(Arrays.asList("a", "b", "c"));
    a.differenceUpdate(context.set(Arrays.asList("b")));
    assertEquals(a.size(), 2);
    assertFalse(a.contains(context.str("b")));
  }

  @Test
  public void testDiscard()
  {
    PySet set = context.set(Arrays.asList("a", "b"));
    set.discard(context.str("a"));
    assertEquals(set.size(), 1);
    // discarding a missing element is a no-op, not an error
    set.discard(context.str("z"));
    assertEquals(set.size(), 1);
  }

  @Test
  public void testIntersect()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    PySet b = context.set(Arrays.asList("b", "c"));
    PySet result = a.intersection(b);
    assertEquals(result.size(), 1);
    assertTrue(result.contains(context.str("b")));
  }

  @Test
  public void testIntersectionUpdate()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    a.intersectionUpdate(context.set(Arrays.asList("b", "c")));
    assertEquals(a.size(), 1);
    assertTrue(a.contains(context.str("b")));
  }

  @Test
  public void testIsDisjoint()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    PySet b = context.set(Arrays.asList("c", "d"));
    PySet c = context.set(Arrays.asList("b", "z"));
    assertTrue(a.isDisjoint(b));
    assertFalse(a.isDisjoint(c));
  }

  @Test
  public void testIsSubset()
  {
    PySet a = context.set(Arrays.asList("a"));
    PySet b = context.set(Arrays.asList("a", "b"));
    assertTrue(a.isSubset(b));
    assertFalse(b.isSubset(a));
  }

  @Test
  public void testIsSuperset()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    PySet b = context.set(Arrays.asList("a"));
    assertTrue(a.isSuperset(b));
    assertFalse(b.isSuperset(a));
  }

  @Test
  public void testPop()
  {
    PySet set = context.set(Arrays.asList("a"));
    PyObject popped = set.pop();
    assertEquals(popped.toString(), "a");
    assertTrue(set.isEmpty());
  }

  @Test
  public void testRemove()
  {
    PySet set = context.set(Arrays.asList("a", "b"));
    assertTrue(set.remove(context.str("a")));
    assertEquals(set.size(), 1);
    assertFalse(set.remove(context.str("z")));
  }

  @Test
  public void testRemoveAll()
  {
    PySet set = context.set(Arrays.asList("a", "b", "c"));
    assertTrue(set.removeAll(Arrays.asList(context.str("a"), context.str("c"))));
    assertEquals(set.size(), 1);
    assertTrue(set.contains(context.str("b")));
  }

  @Test
  public void testRetainAll()
  {
    PySet set = context.set(Arrays.asList("a", "b", "c"));
    assertTrue(set.retainAll(Arrays.asList(context.str("b"))));
    assertEquals(set.size(), 1);
    assertTrue(set.contains(context.str("b")));
  }

  @Test
  public void testSymmetricDifference()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    PySet b = context.set(Arrays.asList("b", "c"));
    PySet result = a.symmetricDifference(b);
    assertEquals(result.size(), 2);
    assertTrue(result.contains(context.str("a")));
    assertTrue(result.contains(context.str("c")));
  }

  @Test
  public void testSymmetricDifferenceUpdate()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    a.symmetricDifferenceUpdate(context.set(Arrays.asList("b", "c")));
    assertEquals(a.size(), 2);
    assertTrue(a.contains(context.str("a")));
    assertTrue(a.contains(context.str("c")));
  }

  @Test
  public void testToList()
  {
    PySet set = context.set(Arrays.asList("a", "b"));
    assertEquals(set.toList().size(), 2);
  }

  @Test
  public void testUnion()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    PySet b = context.set(Arrays.asList("b", "c"));
    PySet result = a.union(b);
    assertEquals(result.size(), 3);
  }

  @Test
  public void testUnionUpdate()
  {
    PySet a = context.set(Arrays.asList("a", "b"));
    a.unionUpdate(context.set(Arrays.asList("b", "c")));
    assertEquals(a.size(), 3);
  }

  @Test
  public void testUpdate()
  {
    PySet a = context.set(Arrays.asList("a"));
    a.update(Arrays.asList(context.str("b"), context.str("c")));
    assertEquals(a.size(), 3);
  }

  @Test
  public void testEquals()
  {
    // Python sets are mutable and therefore unhashable, so hashCode() falls
    // back to id() and is not expected to be consistent across equal sets -
    // only equals() is meaningful here.
    PySet a = context.set(Arrays.asList("a", "b"));
    PySet b = context.set(Arrays.asList("b", "a"));
    assertEquals(a, b);
  }

}
