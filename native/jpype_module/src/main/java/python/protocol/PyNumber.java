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
package python.protocol;

import python.lang.PyObject;

/**
 * Interface representing Python numeric operations.
 *
 * This interface provides methods to perform arithmetic operations, type
 * conversions, and other behaviors that mimic Python's numeric protocol. It is
 * designed to facilitate interoperability between Java and Python numeric
 * types, enabling seamless integration of Python-like numeric behavior in Java
 * applications.
 *
 * Each method corresponds to a Python numeric operation (e.g., addition,
 * subtraction, multiplication) and supports operations with Python objects,
 * primitive types, and in-place modifications.
 */
public interface PyNumber extends PyObject, Comparable<Number>
{

  // Addition operations
  /**
   * Adds the given PyObject to this object.
   *
   * @param other The PyObject to add.
   * @return A new PyObject representing the result of the addition.
   */
  PyObject add(PyObject other);

  /**
   * Adds the given long value to this object.
   *
   * @param value The long value to add.
   * @return A new PyObject representing the result of the addition.
   */
  PyObject add(long value);

  /**
   * Adds the given double value to this object.
   *
   * @param value The double value to add.
   * @return A new PyObject representing the result of the addition.
   */
  PyObject add(double value);

  /**
   * Performs in-place addition with the given PyObject.
   *
   * @param other The PyObject to add.
   * @return The updated PyObject after in-place addition.
   */
  PyObject addInPlace(PyObject other);

  /**
   * Performs in-place addition with the given long value.
   *
   * @param value The long value to add.
   * @return The updated PyObject after in-place addition.
   */
  PyObject addInPlace(long value);

  /**
   * Performs in-place addition with the given double value.
   *
   * @param value The double value to add.
   * @return The updated PyObject after in-place addition.
   */
  PyObject addInPlace(double value);

  // Division operations
  /**
   * Divides this object by the given PyObject.
   *
   * @param other The PyObject to divide by.
   * @return A new PyObject representing the result of the division.
   */
  PyObject divide(PyObject other);

  /**
   * Divides this object by the given long value.
   *
   * @param value The long value to divide by.
   * @return A new PyObject representing the result of the division.
   */
  PyObject divide(long value);

  /**
   * Divides this object by the given double value.
   *
   * @param value The double value to divide by.
   * @return A new PyObject representing the result of the division.
   */
  PyObject divide(double value);

  /**
   * Performs in-place division by the given PyObject.
   *
   * @param other The PyObject to divide by.
   * @return The updated PyObject after in-place division.
   */
  PyObject divideInPlace(PyObject other);

  /**
   * Performs in-place division by the given long value.
   *
   * @param value The long value to divide by.
   * @return The updated PyObject after in-place division.
   */
  PyObject divideInPlace(long value);

  /**
   * Performs in-place division by the given double value.
   *
   * @param value The double value to divide by.
   * @return The updated PyObject after in-place division.
   */
  PyObject divideInPlace(double value);

  // Division with remainder (modulus)
  /**
   * Performs division with remainder (divmod) operation with the given
   * PyObject.
   *
   * @param other The PyObject to divide by.
   * @return A PyObject representing the result of the divmod operation.
   */
  PyObject divideWithRemainder(PyObject other);

  // Matrix multiplication
  /**
   * Performs matrix multiplication with the given PyObject.
   *
   * @param other The PyObject to multiply with.
   * @return A PyObject representing the result of the matrix multiplication.
   */
  PyObject matrixMultiply(PyObject other);

  // Multiplication operations
  /**
   * Multiplies this object by the given PyObject.
   *
   * @param other The PyObject to multiply with.
   * @return A new PyObject representing the result of the multiplication.
   */
  PyObject multiply(PyObject other);

  /**
   * Multiplies this object by the given long value.
   *
   * @param value The long value to multiply with.
   * @return A new PyObject representing the result of the multiplication.
   */
  PyObject multiply(long value);

  /**
   * Multiplies this object by the given double value.
   *
   * @param value The double value to multiply with.
   * @return A new PyObject representing the result of the multiplication.
   */
  PyObject multiply(double value);

  /**
   * Performs in-place multiplication with the given PyObject.
   *
   * @param other The PyObject to multiply with.
   * @return The updated PyObject after in-place multiplication.
   */
  PyObject multiplyInPlace(PyObject other);

  /**
   * Performs in-place multiplication with the given long value.
   *
   * @param value The long value to multiply with.
   * @return The updated PyObject after in-place multiplication.
   */
  PyObject multiplyInPlace(long value);

  /**
   * Performs in-place multiplication with the given double value.
   *
   * @param value The double value to multiply with.
   * @return The updated PyObject after in-place multiplication.
   */
  PyObject multiplyInPlace(double value);

  // Logical negation
  /**
   * Performs logical negation (NOT operation) on this object.
   *
   * @return A boolean representing the negated value.
   */
  boolean negate();

  // Exponentiation
  /**
   * Raises this object to the power of the given PyObject.
   *
   * @param exponent The PyObject representing the exponent.
   * @return A PyObject representing the result of the exponentiation.
   */
  PyObject power(PyObject exponent);

  // Remainder (modulus)
  /**
   * Computes the remainder (modulus) of this object divided by the given
   * PyObject.
   *
   * @param other The PyObject to divide by.
   * @return A PyObject representing the result of the modulus operation.
   */
  PyObject modulus(PyObject other);

  // Subtraction operations
  /**
   * Subtracts the given PyObject from this object.
   *
   * @param other The PyObject to subtract.
   * @return A new PyObject representing the result of the subtraction.
   */
  PyObject subtract(PyObject other);

  /**
   * Subtracts the given long value from this object.
   *
   * @param value The long value to subtract.
   * @return A new PyObject representing the result of the subtraction.
   */
  PyObject subtract(long value);

  /**
   * Subtracts the given double value from this object.
   *
   * @param value The double value to subtract.
   * @return A new PyObject representing the result of the subtraction.
   */
  PyObject subtract(double value);

  /**
   * Performs in-place subtraction with the given PyObject.
   *
   * @param other The PyObject to subtract.
   * @return The updated PyObject after in-place subtraction.
   */
  PyObject subtractInPlace(PyObject other);

  /**
   * Performs in-place subtraction with the given long value.
   *
   * @param value The long value to subtract.
   * @return The updated PyObject after in-place subtraction.
   */
  PyObject subtractInPlace(long value);

  /**
   * Performs in-place subtraction with the given double value.
   *
   * @param value The double value to subtract.
   * @return The updated PyObject after in-place subtraction.
   */
  PyObject subtractInPlace(double value);

  // Conversion methods
  /**
   * Converts this object to a boolean value.
   *
   * @return A boolean representation of this object.
   */
  boolean toBoolean();

  /**
   * Converts this object to a double value.
   *
   * @return A double representation of this object.
   */
  double toDouble();

  /**
   * Converts this object to an integer value.
   *
   * @return An integer representation of this object.
   */
  int toInteger();

  /**
   * Computes the absolute value of this object.
   *
   * @return A PyObject representing the absolute value.
   */
  PyObject abs();

  /**
   * Computes the negation (unary minus) of this object.
   *
   * @return A PyObject representing the negated value.
   */
  PyObject negateValue();

  /**
   * Computes the positive value (unary plus) of this object.
   *
   * @return A PyObject representing the positive value.
   */
  PyObject positive();

  /**
   * Performs floor division with the given PyObject.
   *
   * @param other The PyObject to divide by.
   * @return A PyObject representing the result of the floor division.
   */
  PyObject floorDivide(PyObject other);

  /**
   * Performs floor division with the given long value.
   *
   * @param value The long value to divide by.
   * @return A PyObject representing the result of the floor division.
   */
  PyObject floorDivide(long value);

  /**
   * Performs floor division with the given double value.
   *
   * @param value The double value to divide by.
   * @return A PyObject representing the result of the floor division.
   */
  PyObject floorDivide(double value);

  @Override
  int compareTo(Number o);
}
