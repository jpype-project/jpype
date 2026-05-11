// --- file: python/exception/PyTypeError.java ---
package python.exception;

import python.lang.PyExc;

public class PyTypeError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyTypeError(PyExc base)
  {
    super(base);
  }
}
