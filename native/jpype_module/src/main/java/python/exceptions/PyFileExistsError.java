// --- file: python/exception/PyFileExistsError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyFileExistsError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyFileExistsError(PyExc base)
  {
    super(base);
  }
}
