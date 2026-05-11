// --- file: python/exception/PyAttributeError.java ---
package python.exception;

import python.lang.PyExc;

public class PyAttributeError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyAttributeError(PyExc base)
  {
    super(base);
  }
}
