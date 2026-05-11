// --- file: python/exception/PyArithmeticError.java ---
package python.exception;

import python.lang.PyExc;

public class PyArithmeticError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyArithmeticError(PyExc base)
  {
    super(base);
  }
}
