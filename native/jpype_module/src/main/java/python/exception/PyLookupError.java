// --- file: python/exception/PyLookupError.java ---
package python.exception;

import python.lang.PyExc;

public class PyLookupError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyLookupError(PyExc base)
  {
    super(base);
  }
}
