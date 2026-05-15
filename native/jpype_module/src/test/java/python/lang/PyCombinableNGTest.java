package python.lang;

import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

public class PyCombinableNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    if (!Interpreter.getInstance().isStarted())
      Interpreter.getInstance().start(new String[0]);
  }

  private PyDict dictOf(Object... items)
  {
    PyDict dict = PyBuiltIn.dict();
    for (int i = 0; i < items.length; i += 2)
      dict.putAny(items[i], items[i + 1]);
    return dict;
  }

  @Test
  public void testOrWithDisjointDicts()
  {
    PyDict left = dictOf("a", PyInt.of(1));
    PyDict right = dictOf("b", PyInt.of(2));

    PyObject result = left.or(right);

    assertNotNull(result);
    assertTrue(result instanceof PyDict);

    PyDict out = (PyDict) result;
    assertEquals(out.size(), 2);
    assertEquals(out.get("a").toString(), "1");
    assertEquals(out.get("b").toString(), "2");
  }

  @Test
  public void testOrWithOverlappingKeysRightWins()
  {
    PyDict left = dictOf("a", PyInt.of(1), "b", PyInt.of(2));
    PyDict right = dictOf("b", PyInt.of(99), "c", PyInt.of(3));

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 3);
    assertEquals(out.get("a").toString(), "1");
    assertEquals(out.get("b").toString(), "99");
    assertEquals(out.get("c").toString(), "3");
  }

  @Test
  public void testOrDoesNotModifyLeftOperand()
  {
    PyDict left = dictOf("a", PyInt.of(1));
    PyDict right = dictOf("a", PyInt.of(10), "b", PyInt.of(2));

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
    PyDict left = dictOf("a", PyInt.of(1));
    PyDict right = dictOf("b", PyInt.of(2));

    PyDict out = (PyDict) left.or(right);

    assertEquals(right.size(), 1);
    assertEquals(right.get("b").toString(), "2");

    assertEquals(out.size(), 2);
  }

  @Test
  public void testOrWithEmptyLeft()
  {
    PyDict left = PyBuiltIn.dict();
    PyDict right = dictOf("x", PyInt.of(7));

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 1);
    assertEquals(out.get("x").toString(), "7");
  }

  @Test
  public void testOrWithEmptyRight()
  {
    PyDict left = dictOf("x", PyInt.of(7));
    PyDict right = PyBuiltIn.dict();

    PyDict out = (PyDict) left.or(right);

    assertEquals(out.size(), 1);
    assertEquals(out.get("x").toString(), "7");
  }

  @Test
  public void testOrWithBothEmpty()
  {
    PyDict left = PyBuiltIn.dict();
    PyDict right = PyBuiltIn.dict();

    PyDict out = (PyDict) left.or(right);

    assertNotNull(out);
    assertTrue(out.isEmpty());
  }

  @Test
  public void testOrResultIsNewObject()
  {
    PyDict left = dictOf("a", PyInt.of(1));
    PyDict right = dictOf("b", PyInt.of(2));

    PyDict out = (PyDict) left.or(right);

    assertNotSame(out, left);
    assertNotSame(out, right);
  }
}
