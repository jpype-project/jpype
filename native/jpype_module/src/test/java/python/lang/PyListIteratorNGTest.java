// --- file: python/lang/PyListIteratorNGTest.java ---
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

import java.util.ListIterator;
import java.util.NoSuchElementException;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyListIteratorNGTest extends PyTestHarness
{

  @Test
  public void testHasNext()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    assertTrue(it.hasNext());
  }

  @Test
  public void testNext()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    assertEquals(it.next().toString(), "a");
    assertEquals(it.next().toString(), "b");
    assertEquals(it.next().toString(), "c");
    assertFalse(it.hasNext());
  }

  @Test(expectedExceptions = NoSuchElementException.class)
  public void testNextPastEndThrows()
  {
    PyList list = context.listFromObjects("a");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    it.next();
    it.next();
  }

  @Test
  public void testHasPrevious()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    assertFalse(it.hasPrevious());
    it.next();
    assertTrue(it.hasPrevious());
  }

  @Test
  public void testPrevious()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = new PyListIterator(list, 2);
    assertEquals(it.previous().toString(), "b");
    assertEquals(it.previous().toString(), "a");
  }

  @Test
  public void testNextIndex()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    assertEquals(it.nextIndex(), 0);
    it.next();
    assertEquals(it.nextIndex(), 1);
  }

  @Test
  public void testPreviousIndex()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = new PyListIterator(list, 1);
    assertEquals(it.previousIndex(), 0);
  }

  @Test
  public void testSet()
  {
    PyList list = context.listFromObjects("a", "b");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    it.next();
    it.set(context.str("z"));
    assertEquals(list.get(0).toString(), "z");
  }

  @Test
  public void testRemove()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    ListIterator<PyObject> it = new PyListIterator(list, 0);
    it.next();
    it.remove();
    assertEquals(list.size(), 2);
    assertEquals(list.get(0).toString(), "b");
  }

  @Test
  public void testAdd()
  {
    PyList list = context.listFromObjects("a", "c");
    ListIterator<PyObject> it = new PyListIterator(list, 1);
    it.add(context.str("b"));
    assertEquals(list.size(), 3);
    assertEquals(list.get(1).toString(), "b");
  }

  @Test
  public void testStartAtNonZeroIndex()
  {
    PyList list = context.listFromObjects("a", "b", "c");
    ListIterator<PyObject> it = new PyListIterator(list, 1);
    assertEquals(it.next().toString(), "b");
  }

}
