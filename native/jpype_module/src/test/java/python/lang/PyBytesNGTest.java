// --- file: python/lang/PyBytesNGTest.java ---
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.Test;

public class PyBytesNGTest extends PyTestHarness
{

  @Test
  public void testCreateIsZeroFilled()
  {
    PyBytes instance = context.bytes(3);

    assertEquals(instance.get(0).toNumber().intValue(), 0);
    assertEquals(instance.get(1).toNumber().intValue(), 0);
    assertEquals(instance.get(2).toNumber().intValue(), 0);
  }

  @Test
  public void testCreateWithLength()
  {
    PyBytes instance = context.bytes(4);

    assertNotNull(instance);
    assertEquals(instance.size(), 4);
  }

  @Test
  public void testCreateWithZeroLength()
  {
    PyBytes instance = context.bytes(0);

    assertNotNull(instance);
    assertEquals(instance.size(), 0);
    assertTrue(instance.isEmpty());
  }

  @Test
  public void testDecodeAscii()
  {
    PyBytes instance = context.bytesFromHex("414243");

    PyString decoded = instance.decode("ascii", null);

    assertNotNull(decoded);
    assertEquals(decoded.toString(), "ABC");
  }

  @Test
  public void testDecodeUtf8()
  {
    PyBytes instance = context.bytesFromHex("48656c6c6f");

    PyString decoded = instance.decode("utf-8", null);

    assertNotNull(decoded);
    assertEquals(decoded.toString(), "Hello");
  }

  @Test
  public void testFromHex()
  {
    PyBytes instance = context.bytesFromHex("48656c6c6f");

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
    context.bytesFromHex("not_hex");
  }

  @Test
  public void testGetByIndex()
  {
    PyBytes instance = context.bytesFromHex("414243");

    assertEquals(instance.get(0).toNumber().intValue(), 65);
    assertEquals(instance.get(1).toNumber().intValue(), 66);
    assertEquals(instance.get(2).toNumber().intValue(), 67);
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testGetOutOfBoundsThrows()
  {
    PyBytes instance = context.bytesFromHex("41");
    instance.get(5);
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testRemoveUnsupported()
  {
    PyBytes instance = context.bytesFromHex("414243");
    instance.remove(1);
  }

  @Test(expectedExceptions = UnsupportedOperationException.class)
  public void testSetUnsupported()
  {
    PyBytes instance = context.bytesFromHex("414243");
    instance.set(1, context.$int(90));
  }

  @Test
  public void testSize()
  {
    PyBytes instance = context.bytesFromHex("414243");
    assertEquals(instance.size(), 3);
  }
}
