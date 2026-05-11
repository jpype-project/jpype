// --- file: python/exception/PyZeroDivisionError.java ---
package python.exception;

import python.lang.PyExc;

public class PyZeroDivisionError extends PyArithmeticError
{

  private static final long serialVersionUID = 1L;

  public PyZeroDivisionError(PyExc base)
  {
    super(base);
  }
}
