// --- file: python/exception/PyADirectionError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyIsADirectoryError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyIsADirectoryError(PyExc base)
  {
    super(base);
  }
}
