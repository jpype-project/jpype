// --- file: python/exception/PyNotImplementedError.java ---
package python.exception;

import python.lang.PyExc;

public class PyNotImplementedError extends PyRuntimeError
{

  private static final long serialVersionUID = 1L;

  public PyNotImplementedError(PyExc base)
  {
    super(base);
  }
}
