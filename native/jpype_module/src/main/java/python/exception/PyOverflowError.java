package python.exception;

import python.lang.PyExc;

public class PyOverflowError extends PyArithmeticError
{

  public PyOverflowError(PyExc base)
  {
    super(base);
  }
}
