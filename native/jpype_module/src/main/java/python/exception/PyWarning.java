// --- file: python/exception/PyWarning.java ---
package python.exception;

import python.lang.PyExc;

public class PyWarning extends PyException
{

  private static final long serialVersionUID = 1L;

  public PyWarning(PyExc base)
  {
    super(base);
  }
}
