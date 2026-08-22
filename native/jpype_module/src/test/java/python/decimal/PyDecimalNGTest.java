// --- file: python/decimal/PyDecimalNGTest.java ---
package python.decimal;

import java.math.BigDecimal;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

public class PyDecimalNGTest extends PyTestHarness
{

  @Test
  public void testConstructionAndToString()
  {
    PyDecimal d = Decimal.using(context).decimal("19.99");

    assertEquals(d.toString(), "19.99");
  }

  @Test
  public void testArithmetic()
  {
    Decimal decimal = Decimal.using(context);
    PyDecimal a = decimal.decimal("5");
    PyDecimal b = decimal.decimal("3");

    assertEquals(a.add(b).toString(), "8");
    assertEquals(a.subtract(b).toString(), "2");
    assertEquals(a.multiply(b).toString(), "15");
    assertEquals(a.negate().toString(), "-5");
    assertEquals(decimal.decimal("-5").abs().toString(), "5");
  }

  @Test
  public void testDivide()
  {
    Decimal decimal = Decimal.using(context);
    PyDecimal a = decimal.decimal("10");
    PyDecimal b = decimal.decimal("4");

    assertEquals(a.divide(b).toString(), "2.5");
  }

  @Test(expectedExceptions = RuntimeException.class)
  public void testDivideByZeroThrows()
  {
    Decimal decimal = Decimal.using(context);
    decimal.decimal("1").divide(decimal.decimal("0"));
  }

  @Test
  public void testSpecialValuePredicates()
  {
    Decimal decimal = Decimal.using(context);
    PyDecimal nan = decimal.decimal("NaN");
    PyDecimal inf = decimal.decimal("Infinity");
    PyDecimal finite = decimal.decimal("1.5");

    assertTrue(nan.isNaN());
    assertFalse(nan.isFinite());
    assertTrue(inf.isInfinite());
    assertFalse(inf.isFinite());
    assertTrue(finite.isFinite());
    assertFalse(finite.isNaN());
    assertFalse(finite.isInfinite());
  }

  @Test
  public void testCompareToAndEquals()
  {
    Decimal decimal = Decimal.using(context);
    PyDecimal a = decimal.decimal("1.1");
    PyDecimal b = decimal.decimal("1.2");
    PyDecimal aAgain = decimal.decimal("1.1");

    assertTrue(a.compareTo(b) < 0);
    assertTrue(b.compareTo(a) > 0);
    assertEquals(a.compareTo(aAgain), 0);
    assertEquals(a, aAgain);
  }

  @Test
  public void testToBigDecimalRoundTrip()
  {
    Decimal decimal = Decimal.using(context);
    PyDecimal d = decimal.decimal("123.456000");

    BigDecimal bd = d.toBigDecimal();

    assertEquals(bd, new BigDecimal("123.456000"));
  }

  @Test
  public void testDecimalFromBigDecimal()
  {
    Decimal decimal = Decimal.using(context);
    BigDecimal bd = new BigDecimal("42.50");

    PyDecimal d = decimal.decimalFromBigDecimal(bd);

    assertEquals(d.toString(), "42.50");
    assertEquals(d.toBigDecimal(), bd);
  }

}
