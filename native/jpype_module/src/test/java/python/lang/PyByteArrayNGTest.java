// --- file: python/lang/PyByteArrayNGTest.java ---
package python.lang;

import java.util.Arrays;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyByteArrayNGTest extends PyTestHarness
{

  @Test
  public void testCreateIsZeroFilled()
  {
    PyByteArray instance = context.bytearray(3);

    assertEquals(instance.get(0).toNumber().intValue(), 0);
    assertEquals(instance.get(1).toNumber().intValue(), 0);
    assertEquals(instance.get(2).toNumber().intValue(), 0);
  }

  @Test
  public void testCreateWithLength()
  {
    PyByteArray instance = context.bytearray(10);

    assertNotNull(instance);
    assertEquals(instance.size(), 10);
  }

  @Test
  public void testCreateWithZeroLength()
  {
    PyByteArray instance = context.bytearray(0);
    assertNotNull(instance);
    assertEquals(instance.size(), 0);
    assertTrue(instance.isEmpty());
  }

  @Test
  public void testDecodeUtf8()
  {
    PyByteArray instance = context.bytearrayFromHex("48656c6c6f");

    PyObject decoded = instance.decode(context.str("utf-8"), null);

    assertNotNull(decoded);
    assertEquals(decoded.toString(), "Hello");
  }

  @Test
  public void testFromHex()
  {
    PyByteArray instance = context.bytearrayFromHex("48656c6c6f");

    assertNotNull(instance);
    assertEquals(instance.size(), 5);
    assertEquals(instance.get(0).toNumber().intValue(), 72);
    assertEquals(instance.get(1).toNumber().intValue(), 101);
    assertEquals(instance.get(2).toNumber().intValue(), 108);
    assertEquals(instance.get(3).toNumber().intValue(), 108);
    assertEquals(instance.get(4).toNumber().intValue(), 111);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testFromHexInvalidThrows()
  {
    context.bytearrayFromHex("this_is_not_hex");
  }

  @Test
  public void testGetByIndex()
  {
    PyByteArray instance = context.bytearrayFromHex("4142");

    PyInt value0 = instance.get(0);
    PyInt value1 = instance.get(1);

    assertEquals(value0.toNumber().intValue(), 65);
    assertEquals(value1.toNumber().intValue(), 66);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testGetOutOfBoundsThrows()
  {
    PyByteArray instance = context.bytearrayFromHex("41");
    instance.get(5);
  }

  @Test(expectedExceptions = IllegalArgumentException.class)
  public void testGetWithTupleSubscriptRejected()
  {
    PyByteArray instance = context.bytearrayFromHex("414243");
    instance.get(context.slice(0, 1), context.slice(1, 2));
  }

  @Test
  public void testOfBuffer()
  {
    PyBytes buffer = context.bytesFromHex("414243");
    PyByteArray instance = context.bytearray(buffer);

    assertNotNull(instance);
    assertEquals(instance.size(), 3);
    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 66);
    assertEquals(instance.get(2).toNumber().intValue(), 67);
  }

  @Test
  public void testOfIterable()
  {
    PyByteArray instance = context.bytearray(Arrays.asList(
            context.$int(65), context.$int(66), context.$int(67)));

    assertNotNull(instance);
    assertEquals(instance.size(), 3);
    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 66);
    assertEquals(instance.get(2).toNumber().intValue(), 67);
  }

  @Test
  public void testRemoveByIndex()
  {
    PyByteArray instance = context.bytearrayFromHex("414243");

    PyInt removed = instance.remove(1);

    assertNotNull(removed);
    assertEquals(removed.toNumber().intValue(), 66);
    assertEquals(instance.size(), 2);
    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 67);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testRemoveOutOfBoundsThrows()
  {
    PyByteArray instance = context.bytearrayFromHex("41");
    instance.remove(5);
  }

  @Test
  public void testSetByIndex()
  {
    PyByteArray instance = context.bytearrayFromHex("414243");

    PyInt previous = instance.set(1, context.$int(90));

    assertNotNull(previous);
    assertEquals(previous.toNumber().intValue(), 66);
    assertEquals(instance.get(1).toNumber().intValue(), 90);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testSetOutOfBoundsThrows()
  {
    PyByteArray instance = context.bytearrayFromHex("41");
    instance.set(5, context.$int(1));
  }
}
