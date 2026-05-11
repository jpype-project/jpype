// --- file: python/exception/PyFileNotFoundError.java ---
package python.exception;

import python.lang.PyExc;

public class PyFileNotFoundError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyFileNotFoundError(PyExc base)
  {
    super(base);
  }
}
