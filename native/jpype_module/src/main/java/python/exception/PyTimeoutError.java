// --- file: python/exception/PyTimeoutError.java ---
package python.exception;

import python.lang.PyExc;

public class PyTimeoutError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyTimeoutError(PyExc base)
  {
    super(base);
  }
}
