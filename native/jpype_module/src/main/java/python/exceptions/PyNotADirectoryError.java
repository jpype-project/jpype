// --- file: python/exception/PyNotADirectoryError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyNotADirectoryError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyNotADirectoryError(PyExc base)
  {
    super(base);
  }
}
