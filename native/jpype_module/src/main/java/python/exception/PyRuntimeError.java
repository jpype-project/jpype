// --- file: python/exception/PyRuntimeError.java ---
package python.exception;

import python.lang.PyExc;

public class PyRuntimeError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyRuntimeError(PyExc base)
  {
    super(base);
  }
}
