// --- file: python/exception/PyNameError.java ---
package python.exception;

import python.lang.PyExc;

public class PyNameError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyNameError(PyExc base)
  {
    super(base);
  }
}
