// --- file: python/decimal/package-info.java ---
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
/**
 * Java front-end for Python's {@code decimal} module, following the same
 * conventions as {@link python.lang}, {@link python.io},
 * {@link python.collections}, {@link python.datetime}, and
 * {@link python.pathlib}.
 *
 * Start from {@link Decimal#using(python.lang.PyBuiltIn)
 * Decimal.using(context)} to construct instances of the concrete type in
 * this package: {@link Decimal#decimal(String) decimal(...)} for a
 * {@code decimal.Decimal}. The returned {@link PyDecimal} is a normal Java
 * interface backed by the real Python object, so its methods behave
 * exactly as they would in Python.
 *
 * <p>
 * This first cut covers {@code Decimal} itself: construction, the four
 * basic arithmetic operators ({@link PyDecimal#add}, {@link
 * PyDecimal#subtract}, {@link PyDecimal#multiply}, {@link
 * PyDecimal#divide}), sign/special-value predicates ({@link
 * PyDecimal#isNaN}, {@link PyDecimal#isInfinite}, {@link
 * PyDecimal#isFinite}), comparison, and promotion to/from
 * {@link java.math.BigDecimal}. It does not cover {@code decimal.Context},
 * {@code localcontext}, or the rounding-mode constants, which are
 * process-global configuration state rather than a value type.
 *
 * <p>
 * {@link PyDecimal} also offers a promotion default method to the
 * standard Java equivalent, {@link PyDecimal#toBigDecimal()}, the reverse
 * of the conversion JPype's forward-bridge {@code JConversion} customizer
 * for {@code java.math.BigDecimal} already uses (see
 * {@code jpype/protocol.py}). This plan's {@link PyDecimal} instead gives
 * Java code a typed front-end object for a {@code Decimal} value received
 * *from* Python, which the forward-bridge customizer does not provide.
 */
package python.decimal;
