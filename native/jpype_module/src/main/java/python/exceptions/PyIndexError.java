// --- file: python/exception/PyIndexError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyIndexError extends PyLookupError
{

  private static final long serialVersionUID = 1L;

  public PyIndexError(PyExc base)
  {
    super(base);
  }
}
