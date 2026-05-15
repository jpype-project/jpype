package python.lang;

import java.util.Arrays;
import org.jpype.bridge.Interpreter;
import static org.testng.Assert.*;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

public class PyByteArrayNGTest
{

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    if (!Interpreter.getInstance().isStarted())
      Interpreter.getInstance().start(new String[0]);
  }

  @Test
  public void testCreateWithZeroLength()
  {
    PyByteArray instance = PyByteArray.create(0);

    assertNotNull(instance);
    assertEquals(instance.size(), 0);
    assertTrue(instance.isEmpty());
  }

  @Test
  public void testCreateWithLength()
  {
    PyByteArray instance = PyByteArray.create(10);

    assertNotNull(instance);
    assertEquals(instance.size(), 10);
  }

  @Test
  public void testCreateIsZeroFilled()
  {
    PyByteArray instance = PyByteArray.create(3);

    assertEquals(instance.get(0).toNumber().intValue(), 0);
    assertEquals(instance.get(1).toNumber().intValue(), 0);
    assertEquals(instance.get(2).toNumber().intValue(), 0);
  }

  @Test
  public void testFromHex()
  {
    PyByteArray instance = PyByteArray.fromHex("48656c6c6f");

    assertNotNull(instance);
    assertEquals(instance.size(), 5);
    assertEquals(instance.get(0).toNumber().intValue(), 72);
    assertEquals(instance.get(1).toNumber().intValue(), 101);
    assertEquals(instance.get(2).toNumber().intValue(), 108);
    assertEquals(instance.get(3).toNumber().intValue(), 108);
    assertEquals(instance.get(4).toNumber().intValue(), 111);
  }

  @Test
  public void testOfIterable()
  {
    PyByteArray instance = PyByteArray.of(Arrays.asList(
            PyInt.of(65), PyInt.of(66), PyInt.of(67)));

    assertNotNull(instance);
    assertEquals(instance.size(), 3);
    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 66);
    assertEquals(instance.get(2).toNumber().intValue(), 67);
  }

  @Test
  public void testOfBuffer()
  {
    PyBytes buffer = PyBytes.fromHex("414243");
    PyByteArray instance = PyByteArray.of(buffer);

    assertNotNull(instance);
    assertEquals(instance.size(), 3);
    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 66);
    assertEquals(instance.get(2).toNumber().intValue(), 67);
  }

  @Test
  public void testGetByIndex()
  {
    PyByteArray instance = PyByteArray.fromHex("4142");

    PyInt value0 = instance.get(0);
    PyInt value1 = instance.get(1);

    assertEquals(value0.toNumber().intValue(), 65);
    assertEquals(value1.toNumber().intValue(), 66);
  }

  @Test
  public void testSetByIndex()
  {
    PyByteArray instance = PyByteArray.fromHex("414243");

    PyInt previous = instance.set(1, PyInt.of(90));

    assertNotNull(previous);
    assertEquals(previous.toNumber().intValue(), 66);
    assertEquals(instance.get(1).toNumber().intValue(), 90);
  }

  @Test
  public void testRemoveByIndex()
  {
    PyByteArray instance = PyByteArray.fromHex("414243");

    PyInt removed = instance.remove(1);

    assertNotNull(removed);
    assertEquals(removed.toNumber().intValue(), 66);
    assertEquals(instance.size(), 2);
    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 67);
  }

  @Test
  public void testDecodeUtf8()
  {
    PyByteArray instance = PyByteArray.fromHex("48656c6c6f");

    PyObject decoded = instance.decode(PyString.from("utf-8"), null);

    assertNotNull(decoded);
    assertEquals(decoded.toString(), "Hello");
  }

  @Test(expectedExceptions = IllegalArgumentException.class)
  public void testGetWithTupleSubscriptRejected()
  {
    PyByteArray instance = PyByteArray.fromHex("414243");
    instance.get(PyBuiltIn.slice(0, 1), PyBuiltIn.slice(1, 2));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testFromHexInvalidThrows()
  {
    PyByteArray.fromHex("this_is_not_hex");
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testGetOutOfBoundsThrows()
  {
    PyByteArray instance = PyByteArray.fromHex("41");
    instance.get(5);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testSetOutOfBoundsThrows()
  {
    PyByteArray instance = PyByteArray.fromHex("41");
    instance.set(5, PyInt.of(1));
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testRemoveOutOfBoundsThrows()
  {
    PyByteArray instance = PyByteArray.fromHex("41");
    instance.remove(5);
  }
}
