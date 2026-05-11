// --- file: python/exception/PyOverflowError.java ---
package python.exception;

import python.lang.PyExc;

public class PyOverflowError extends PyArithmeticError
{

  private static final long serialVersionUID = 1L;

  public PyOverflowError(PyExc base)
  {
    super(base);
  }
}
