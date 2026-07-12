// --- file: python/decimal/PyDecimal.java ---
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
package python.decimal;

import java.math.BigDecimal;
import org.jpype.annotation.Bypass;
import python.lang.PyObject;

/**
 * Java front-end interface for Python's {@code decimal.Decimal} —
 * arbitrary-precision, base-10 arithmetic, the type Python's own
 * {@code decimal} module docs recommend over {@code float} whenever exact
 * decimal representation matters (money, in particular).
 *
 * <p>
 * Create instances via {@link Decimal#using(python.lang.PyBuiltIn)
 * Decimal.using(context)}'s factory methods, not directly. Instances are
 * immutable, matching Python's own {@code Decimal} — every arithmetic
 * method here ({@link #add}, {@link #subtract}, ...) returns a new value
 * rather than mutating the receiver, exactly like Python's {@code +}/
 * {@code -}/... operators do for this type.
 *
 * <p>
 * This first cut covers {@code Decimal} itself: construction, the four
 * basic arithmetic operators, sign/special-value predicates, comparison,
 * and promotion to/from {@link java.math.BigDecimal}. It deliberately does
 * not cover {@code decimal.Context}, {@code localcontext}, or the
 * rounding-mode constants — those are process-global configuration state,
 * not a value type, and are out of scope for this pass; see
 * {@code plan/Decimal.md}.
 */
public interface PyDecimal extends PyObject, Comparable<PyDecimal>
{

  /**
   * @return this value plus {@code other}, matching Python's {@code self + other}.
   * @param other the value to add.
   */
  PyDecimal add(PyDecimal other);

  /**
   * @return this value minus {@code other}, matching Python's {@code self - other}.
   * @param other the value to subtract.
   */
  PyDecimal subtract(PyDecimal other);

  /**
   * @return this value times {@code other}, matching Python's {@code self * other}.
   * @param other the value to multiply by.
   */
  PyDecimal multiply(PyDecimal other);

  /**
   * @return this value divided by {@code other}, matching Python's
   * {@code self / other} (true division, not {@code //}).
   * @param other the value to divide by.
   * @throws RuntimeException (bridged {@code DivisionByZero}) if
   * {@code other} is zero.
   */
  PyDecimal divide(PyDecimal other);

  /**
   * @return the negation of this value, matching Python's {@code -self}.
   */
  PyDecimal negate();

  /**
   * @return the absolute value of this value, matching Python's
   * {@code abs(self)}.
   */
  PyDecimal abs();

  /**
   * @return {@code true} if this value is a NaN (quiet or signaling),
   * matching Python's {@code Decimal.is_nan()}.
   */
  boolean isNaN();

  /**
   * @return {@code true} if this value is positive or negative infinity,
   * matching Python's {@code Decimal.is_infinite()}.
   */
  boolean isInfinite();

  /**
   * @return {@code true} if this value is neither infinite nor a NaN,
   * matching Python's {@code Decimal.is_finite()}.
   */
  boolean isFinite();

  /**
   * Compares this value to another following Python's rich-comparison
   * semantics.
   *
   * @param other the value to compare against.
   * @return a negative integer, zero, or a positive integer as this value
   * is less than, equal to, or greater than {@code other}.
   * @throws RuntimeException (bridged {@code InvalidOperation}) if either
   * value is a NaN — matching Python, where {@code <}/{@code >} on a NaN
   * raises rather than returning a boolean.
   */
  @Override
  int compareTo(PyDecimal other);

  /**
   * Promotes this value to the standard Java {@link BigDecimal}
   * equivalent, via a string round-trip
   * ({@code new BigDecimal(this.toString())}) — the reverse of the
   * conversion JPype's forward-bridge {@code JConversion} customizer for
   * {@code java.math.BigDecimal} already uses (see
   * {@code jpype/protocol.py}).
   *
   * @return a {@link BigDecimal} equal to this value.
   * @throws NumberFormatException if this value is a NaN or infinity —
   * {@code BigDecimal} has no representation for either, matching the
   * same restriction Python's own {@code decimal} documentation notes
   * for interop with types that lack special values.
   */
  @Bypass
  default BigDecimal toBigDecimal()
  {
    return new BigDecimal(toString());
  }

}
