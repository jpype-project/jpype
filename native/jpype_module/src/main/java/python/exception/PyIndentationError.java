// --- file: python/exception/PyIndentationError.java ---
package python.exception;

import python.lang.PyExc;

public class PyIndentationError extends PySyntaxError
{

  private static final long serialVersionUID = 1L;

  public PyIndentationError(PyExc base)
  {
    super(base);
  }
}
