// --- file: python/decimal/Decimal.java ---
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
import python.lang.PyBuiltIn;

/**
 * Entry point for constructing {@code python.decimal} objects, following
 * the same "mini-backend factory interface" convention as
 * {@code python.io.IO}, {@code python.collections.PyCollections},
 * {@code python.datetime.DateTime}, and {@code python.pathlib.Pathlib}.
 *
 * Call {@link #using(PyBuiltIn)} with the interpreter's {@code PyBuiltIn}
 * to get a {@code Decimal} instance, then use its factory methods to
 * create {@code python.decimal} objects:
 *
 * <pre>{@code
 * Decimal decimal = Decimal.using(context);
 * PyDecimal d = decimal.decimal("19.99");
 * }</pre>
 *
 * <p>
 * {@link #decimalFromBigDecimal(BigDecimal)} always goes through
 * {@link #decimal(String)} via {@code BigDecimal.toString()} rather than
 * constructing a {@code Decimal} from the unscaled value/scale directly —
 * same precedent as {@code Pathlib#pathFromFile(java.io.File)} and
 * {@code DateTime}'s {@code java.time} convenience factories, all of which
 * convert through a string and re-enter the primitive factory rather than
 * inventing a second construction path, since a convenience factory needs
 * a distinct name rather than an overload of {@link #decimal(String)}.
 */
public interface Decimal
{

  static Decimal using(PyBuiltIn context)
  {
    return context.getBackend(Decimal.class);
  }

  /**
   * Creates a new {@code decimal.Decimal}, matching Python's
   * {@code Decimal(value)}.
   *
   * @param value the string form of the value, e.g. {@code "19.99"},
   * {@code "1E+2"}, {@code "NaN"}.
   * @return a new {@link PyDecimal} instance.
   */
  PyDecimal decimal(String value);

  /**
   * Creates a new {@code decimal.Decimal} equal to the given
   * {@link BigDecimal}, via {@link BigDecimal#toString()}.
   *
   * @param value the value to convert.
   * @return a new {@link PyDecimal} instance.
   */
  @Bypass
  default PyDecimal decimalFromBigDecimal(BigDecimal value)
  {
    return decimal(value.toString());
  }

}
