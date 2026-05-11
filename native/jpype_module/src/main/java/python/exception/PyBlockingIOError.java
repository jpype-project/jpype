// --- file: python/exception/PyBlockingIOError.java ---
package python.exception;

import python.lang.PyExc;

public class PyBlockingIOError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyBlockingIOError(PyExc base)
  {
    super(base);
  }
}
