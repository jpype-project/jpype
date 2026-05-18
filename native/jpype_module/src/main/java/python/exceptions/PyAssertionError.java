// --- file: python/exception/PyAssertionError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PyAssertionError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyAssertionError(PyExc base)
  {
    super(base);
  }
}
