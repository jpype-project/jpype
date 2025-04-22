package python.exception;

import python.lang.PyExc;

public class PyModuleNotFoundError extends PyImportError
{

  public PyModuleNotFoundError(PyExc base)
  {
    super(base);
  }
}
