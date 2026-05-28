// --- file: python/exception/PyImportError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyImportError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyImportError(PyExc base)
  {
    super(base);
  }
}
