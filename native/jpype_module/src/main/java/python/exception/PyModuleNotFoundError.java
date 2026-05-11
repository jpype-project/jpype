// --- file: python/exception/PyModuleNotFoundError.java ---
package python.exception;

import python.lang.PyExc;

public class PyModuleNotFoundError extends PyImportError
{

  private static final long serialVersionUID = 1L;

  public PyModuleNotFoundError(PyExc base)
  {
    super(base);
  }
}
