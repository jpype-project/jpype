package python.exception;

import python.lang.PyExc;

public class PyInterruptedError extends PyOSError
{

  public PyInterruptedError(PyExc base)
  {
    super(base);
  }
}
