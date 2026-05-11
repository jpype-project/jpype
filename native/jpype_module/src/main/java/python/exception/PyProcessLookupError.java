// --- file: python/exception/PyProcessLookupError.java ---
package python.exception;

import python.lang.PyExc;

public class PyProcessLookupError extends PyOSError
{

  private static final long serialVersionUID = 1L;

  public PyProcessLookupError(PyExc base)
  {
    super(base);
  }
}
