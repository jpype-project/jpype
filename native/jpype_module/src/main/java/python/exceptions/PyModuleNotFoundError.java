// --- file: python/exception/PyModuleNotFoundError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyModuleNotFoundError extends PyImportError
{

  private static final long serialVersionUID = 1L;

  public PyModuleNotFoundError(PyExc base)
  {
    super(base);
  }
}
