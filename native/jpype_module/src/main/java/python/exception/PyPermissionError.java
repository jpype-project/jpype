// --- file: python/exception/PyPermissionError.java ---
package python.exception;

import python.lang.PyExc;

public class PyPermissionError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyPermissionError(PyExc base)
  {
    super(base);
  }
}
