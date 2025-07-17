package python.exception;

import python.lang.PyExc;

public class PyTimeoutError extends PyOSError
{

  public PyTimeoutError(PyExc base)
  {
    super(base);
  }
}
