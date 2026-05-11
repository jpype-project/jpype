// --- file: python/exception/PyConnectionError.java ---
package python.exception;

import python.lang.PyExc;

public class PyConnectionError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyConnectionError(PyExc base)
  {
    super(base);
  }
}
