// --- file: python/exception/PyRecursionError.java ---
package python.exception;

import python.lang.PyExc;

public class PyRecursionError extends PyRuntimeError
{

  private static final long serialVersionUID = 1L;

  public PyRecursionError(PyExc base)
  {
    super(base);
  }
}
