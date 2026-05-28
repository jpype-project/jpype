// --- file: python/exception/PyInterruptedError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyInterruptedError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyInterruptedError(PyExc base)
  {
    super(base);
  }
}
