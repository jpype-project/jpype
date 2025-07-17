package python.exception;

import python.lang.PyExc;

public class PyRecursionError extends PyRuntimeError
{

  public PyRecursionError(PyExc base)
  {
    super(base);
  }
}
