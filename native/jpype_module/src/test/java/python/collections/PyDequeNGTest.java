// --- file: python/collections/PyDequeNGTest.java ---
package python.collections;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyObject;
import python.lang.PyTestHarness;

public class PyDequeNGTest extends PyTestHarness
{

  @Test
  public void testCreateEmpty()
  {
    PyDeque d = PyCollections.using(context).deque();

    assertNotNull(d);
    assertTrue(d.isEmpty());
    assertEquals(d.size(), 0);
    assertNull(d.maxlen());
  }

  @Test
  public void testCreateFromIterable()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(3)));

    assertEquals(d.size(), 3);
    assertFalse(d.isEmpty());
  }

  @Test
  public void testCreateWithMaxlen()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2)), 2);

    assertEquals((int) d.maxlen(), 2);

    d.append(context.$int(3));

    assertEquals(d.size(), 2);
    assertEquals(d.popleft().toString(), "2");
  }

  @Test
  public void testAppendAndAppendleft()
  {
    PyDeque d = PyCollections.using(context).deque();

    d.append(context.$int(1));
    d.appendleft(context.$int(0));
    d.append(context.$int(2));

    assertEquals(d.size(), 3);
    assertEquals(d.peekFirst().toString(), "0");
    assertEquals(d.peekLast().toString(), "2");
  }

  @Test
  public void testPopAndPopleft()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(3)));

    assertEquals(d.pop().toString(), "3");
    assertEquals(d.popleft().toString(), "1");
    assertEquals(d.size(), 1);
  }

  @Test
  public void testAddFirstAddLastRemoveFirstRemoveLast()
  {
    PyDeque d = PyCollections.using(context).deque();

    d.addLast(context.$int(1));
    d.addFirst(context.$int(0));

    assertEquals(d.removeFirst().toString(), "0");
    assertEquals(d.removeLast().toString(), "1");
    assertTrue(d.isEmpty());
  }

  @Test
  public void testPeekFirstPeekLastEmpty()
  {
    PyDeque d = PyCollections.using(context).deque();

    assertNull(d.peekFirst());
    assertNull(d.peekLast());
  }

  @Test
  public void testExtendAndExtendleft()
  {
    PyDeque d = PyCollections.using(context).deque();

    d.extend(context.listFromObjects(context.$int(1), context.$int(2)));
    d.extendleft(context.listFromObjects(context.$int(10), context.$int(20)));

    // extendleft appends each element to the left in turn, so order reverses.
    assertEquals(d.peekFirst().toString(), "20");
    assertEquals(d.size(), 4);
  }

  @Test
  public void testRotate()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(3)));

    d.rotate(1);

    assertEquals(d.peekFirst().toString(), "3");

    d.rotate(-1);

    assertEquals(d.peekFirst().toString(), "1");
  }

  @Test
  public void testCountAndContains()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(1)));

    assertEquals(d.count(context.$int(1)), 2);
    assertTrue(d.contains(context.$int(2)));
    assertFalse(d.contains(context.$int(99)));
  }

  @Test
  public void testIndex()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(3)));

    assertEquals(d.index(context.$int(3)), 2);
  }

  @Test
  public void testInsertAndRemove()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(3)));

    d.insert(1, context.$int(2));

    assertEquals(d.size(), 3);

    d.remove(context.$int(2));

    assertEquals(d.size(), 2);
    assertFalse(d.contains(context.$int(2)));
  }

  @Test
  public void testReverse()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(3)));

    d.reverse();

    assertEquals(d.peekFirst().toString(), "3");
    assertEquals(d.peekLast().toString(), "1");
  }

  @Test
  public void testClear()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2)));

    d.clear();

    assertTrue(d.isEmpty());
  }

  @Test
  public void testIterator()
  {
    PyDeque d = PyCollections.using(context).deque(
            context.listFromObjects(context.$int(1), context.$int(2), context.$int(3)));

    StringBuilder sb = new StringBuilder();
    for (PyObject o : d)
      sb.append(o.toString());

    assertEquals(sb.toString(), "123");
  }
}
