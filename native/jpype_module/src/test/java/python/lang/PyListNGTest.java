// --- file: python/lang/PyListNGTest.java ---
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
import java.util.List;
import java.util.ListIterator;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyListNGTest extends PyTestHarness
{

  @Test
  public void testEmpty()
  {
    PyList list = context.list();
    assertNotNull(list);
    assertTrue(list.isEmpty());
    assertEquals(list.size(), 0);
  }

  @Test
  public void testListFromObjects()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertEquals(list.size(), 3);
    assertEquals(list.get(0).toString(), "a");
    assertEquals(list.get(2).toString(), "c");
  }

  @Test
  public void testAdd()
  {
    PyList list = context.list();
    list.add(context.str("a"));
    assertEquals(list.size(), 1);
    assertEquals(list.get(0).toString(), "a");
  }

  @Test
  public void testAddAtIndex()
  {
    PyList list = context.listFromObjects("a", "c");
    list.add(1, context.str("b"));
    assertEquals(list.size(), 3);
    assertEquals(list.get(1).toString(), "b");
  }

  @Test
  public void testAddAny()
  {
    PyList list = context.list();
    list.addAny(5);
    assertEquals(list.size(), 1);
  }

  @Test
  public void testAddAll()
  {
    PyList list = context.listFromObjects("a");
    list.addAll(Arrays.asList(context.str("b"), context.str("c")));
    assertEquals(list.size(), 3);
    assertEquals(list.get(2).toString(), "c");
  }

  @Test
  public void testAddAllAtIndex()
  {
    PyList list = context.listFromObjects("a", "d");
    list.addAll(1, Arrays.asList(context.str("b"), context.str("c")));
    assertEquals(list.size(), 4);
    assertEquals(list.get(1).toString(), "b");
    assertEquals(list.get(2).toString(), "c");
  }

  @Test
  public void testClear()
  {
    PyList list = context.listFromObjects("a", "b");
    list.clear();
    assertTrue(list.isEmpty());
  }

  @Test
  public void testContains()
  {
    PyList list = context.listFromObjects("a", "b");
    assertTrue(list.contains(context.str("a")));
    assertFalse(list.contains(context.str("z")));
  }

  @Test
  public void testContainsAll()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertTrue(list.containsAll(Arrays.asList(context.str("a"), context.str("b"))));
    assertFalse(list.containsAll(Arrays.asList(context.str("a"), context.str("z"))));
  }

  @Test
  public void testExtend()
  {
    PyList list = context.listFromObjects("a");
    list.extend(Arrays.asList(context.str("b"), context.str("c")));
    assertEquals(list.size(), 3);
  }

  @Test
  public void testIndexOf()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertEquals(list.indexOf(context.str("b")), 1);
    assertEquals(list.indexOf(context.str("z")), -1);
  }

  @Test
  public void testInsert()
  {
    PyList list = context.listFromObjects("a", "d");
    list.insert(1, Arrays.asList(context.str("b"), context.str("c")));
    assertEquals(list.size(), 4);
    assertEquals(list.get(1).toString(), "b");
  }

  @Test
  public void testIterator()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    StringBuilder sb = new StringBuilder();
    for (PyObject obj : list)
      sb.append(obj.toString());
    assertEquals(sb.toString(), "abc");
  }

  @Test
  public void testListIterator()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = list.listIterator();
    assertTrue(it.hasNext());
    assertEquals(it.next().toString(), "a");
    assertEquals(it.next().toString(), "b");
    assertFalse(it.hasNext());
  }

  @Test
  public void testListIteratorFromIndex()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    ListIterator<PyObject> it = list.listIterator(1);
    assertEquals(it.next().toString(), "b");
  }

  @Test(expectedExceptions = IndexOutOfBoundsException.class)
  public void testListIteratorFromInvalidIndex()
  {
    PyList list = context.listFromObjects("a");
    list.listIterator(5);
  }

  @Test
  public void testRemoveObject()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertTrue(list.remove(context.str("b")));
    assertEquals(list.size(), 2);
    assertEquals(list.get(1).toString(), "c");
  }

  @Test
  public void testRemoveAtIndex()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    PyObject removed = list.remove(1);
    assertEquals(removed.toString(), "b");
    assertEquals(list.size(), 2);
  }

  @Test
  public void testRemoveAll()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertTrue(list.removeAll(Arrays.asList(context.str("a"), context.str("c"))));
    assertEquals(list.size(), 1);
    assertEquals(list.get(0).toString(), "b");
  }

  @Test
  public void testRetainAll()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertTrue(list.retainAll(Arrays.asList(context.str("b"))));
    assertEquals(list.size(), 1);
    assertEquals(list.get(0).toString(), "b");
  }

  @Test
  public void testSet()
  {
    PyList list = context.listFromObjects("a", "b");
    PyObject previous = list.set(0, context.str("z"));
    assertEquals(previous.toString(), "a");
    assertEquals(list.get(0).toString(), "z");
  }

  @Test
  public void testSetAny()
  {
    PyList list = context.listFromObjects("a", "b");
    list.setAny((int) 0, (Object) 42);
    assertEquals(list.get(0).toString(), "42");
  }

  @Test
  public void testSize()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    assertEquals(list.size(), 3);
  }

  @Test
  public void testSubList()
  {
    PyList list = context.listFromObjects("a", "b", "c", "d");
    PyList sub = list.subList(1, 3);
    assertEquals(sub.size(), 2);
    assertEquals(sub.get(0).toString(), "b");
    assertEquals(sub.get(1).toString(), "c");
  }

  @Test
  public void testToArray()
  {
    PyList list = context.listFromObjects("a", "b");
    Object[] array = list.toArray();
    assertEquals(array.length, 2);
  }

  @Test
  public void testToTypedArray()
  {
    PyList list = context.listFromObjects("a", "b");
    Object[] array = list.toArray(new Object[0]);
    assertEquals(array.length, 2);
  }

  @Test
  public void testListFromItems()
  {
    List<String> items = Arrays.asList("x", "y");
    PyList list = context.listFromItems(items);
    assertEquals(list.size(), 2);
    assertEquals(list.get(0).toString(), "x");
  }

}
