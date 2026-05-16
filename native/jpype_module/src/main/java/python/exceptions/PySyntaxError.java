// --- file: python/exception/PySyntaxError.java ---
package python.exceptions;

import python.lang.PyExc;

public class PySyntaxError extends PyException
{

  private static final long serialVersionUID = 1L;

  public PySyntaxError(PyExc base)
  {
    super(base);
  }
}
