// --- file: python/exception/PyChildProcessError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyChildProcessError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyChildProcessError(PyExc base)
  {
    super(base);
  }
}
