// --- file: python/lang/PyCombinableNGTest.java ---
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyCombinableNGTest extends PyTestHarness
{

  private PyDict dictOf(Object... items)
  {
    PyDict dict = context.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @Test
  public void testOrDoesNotModifyLeftOperand()
  {
    PyDict left = dictOf("a", context.$int(1));
    PyDict right = dictOf("a", context.$int(10), "b", context.$int(2));

    PyDict out = (PyDict) left.or(right);

    assertEquals(left.size(), 1);
    assertEquals(left.get("a").toString(), "1");

    assertEquals(out.size(), 2);
    assertEquals(out.get("a").toString(), "10");
    assertEquals(out.get("b").toString(), "2");
  }

  @Test
  public void testOrDoesNotModifyRightOperand()
  {
    PyDict left = dictOf("a", context.$int(1));
    PyDict right = dictOf("b", context.$int(2));

    PyDict out = (PyDict) left.or(right);

    assertEquals(right.size(), 1);
    assertEquals(right.get("b").toString(), "2");

    assertEquals(out.size(), 2);
  }

  @Test
  public void testOrResultIsNewObject()
  {
    PyDict left = dictOf("a", context.$int(1));
    PyDict right = dictOf("b", context.$int(2));

    PyDict out = (PyDict) left.or(right);

    assertNotSame(out, left);
    assertNotSame(out, right);
  }

  @Test
  public void testOrWithBothEmpty()
  {
    PyDict left = context.dict();
    PyDict right = context.dict();

    PyDict out = (PyDict) left.or(right);

    assertNotNull(out);
    assertTrue(out.isEmpty());
  }

  @Test
  public void testOrWithDisjointDicts()
  {
    PyDict left = dictOf("a", context.$int(1));
    PyDict right = dictOf("b", context.$int(2));

    PyObject result = left.or(right);

    assertNotNull(result);
    assertTrue(result instanceof PyDict);

    PyDict out = (PyDict) result;
    assertEquals(out.size(), 2);
    assertEquals(out.get("a").toString(), "1");
    assertEquals(out.get("b").toString(), "2");
  }

  @Test
  public void testOrWithEmptyLeft()
  {
    PyDict left = context.dict();
    PyDict right = dictOf("x", context.$int(7));

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 1);
    assertEquals(out.get("x").toString(), "7");
  }

  @Test
  public void testOrWithEmptyRight()
  {
    PyDict left = dictOf("x", context.$int(7));
    PyDict right = context.dict();

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 1);
    assertEquals(out.get("x").toString(), "7");
  }

  @Test
  public void testOrWithOverlappingKeysRightWins()
  {
    PyDict left = dictOf("a", context.$int(1), "b", context.$int(2));
    PyDict right = dictOf("b", context.$int(99), "c", context.$int(3));

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 3);
    assertEquals(out.get("a").toString(), "1");
    assertEquals(out.get("b").toString(), "99");
    assertEquals(out.get("c").toString(), "3");
  }

}
