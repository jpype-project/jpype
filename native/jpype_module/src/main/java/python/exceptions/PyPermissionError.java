// --- file: python/exception/PyPermissionError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyPermissionError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyPermissionError(PyExc base)
  {
    super(base);
  }
}
