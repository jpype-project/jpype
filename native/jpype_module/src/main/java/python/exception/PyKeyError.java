// --- file: python/exception/PyKeyError.java ---
package python.exception;

import python.lang.PyExc;

public class PyKeyError extends PyLookupError
{

  private static final long serialVersionUID = 1L;

  public PyKeyError(PyExc base)
  {
    super(base);
  }
}
