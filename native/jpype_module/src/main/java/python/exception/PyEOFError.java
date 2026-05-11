// --- file: python/exception/PyEOFError.java ---
package python.exception;

import python.lang.PyExc;

public class PyEOFError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyEOFError(PyExc base)
  {
    super(base);
  }
}
