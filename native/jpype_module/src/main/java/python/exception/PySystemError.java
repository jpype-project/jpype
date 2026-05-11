// --- file: python/exception/PySystemError.java ---
package python.exception;

import python.lang.PyExc;

public class PySystemError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PySystemError(PyExc base)
  {
    super(base);
  }
}
