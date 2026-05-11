// --- file: python/exception/PyReferenceError.java ---
package python.exception;

import python.lang.PyExc;

public class PyReferenceError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyReferenceError(PyExc base)
  {
    super(base);
  }
}
