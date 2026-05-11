// --- file: python/exception/PyFloatingPointError.java ---
package python.exception;

import python.lang.PyExc;

public class PyFloatingPointError extends PyArithmeticError
{

  private static final long serialVersionUID = 1L;

  public PyFloatingPointError(PyExc base)
  {
    super(base);
  }
}
