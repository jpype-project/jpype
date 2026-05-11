// --- file: python/exception/PyValueError.java ---
package python.exception;

import python.lang.PyExc;

public class PyValueError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyValueError(PyExc base)
  {
    super(base);
  }
}
