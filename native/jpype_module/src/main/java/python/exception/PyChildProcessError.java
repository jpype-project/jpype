package python.exception;

import python.lang.PyExc;

public class PyChildProcessError extends PyOSError
{

  public PyChildProcessError(PyExc base)
  {
    super(base);
  }
}
