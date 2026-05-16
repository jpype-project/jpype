// --- file: python/exception/PyOSError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyOSError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyOSError(PyExc base)
  {
    super(base);
  }
}
