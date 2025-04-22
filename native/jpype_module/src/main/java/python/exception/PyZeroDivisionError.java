package python.exception;

import python.lang.PyExc;

public class PyZeroDivisionError extends PyArithmeticError
{

  public PyZeroDivisionError(PyExc base)
  {
    super(base);
  }
}
