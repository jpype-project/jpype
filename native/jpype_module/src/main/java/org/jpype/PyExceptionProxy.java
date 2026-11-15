package org.jpype;

/**
 *
 * @author nelson85
 */
public class PyExceptionProxy extends RuntimeException
{

  // public: read from org.jpype.internal.Support (a different package),
  // which took over getExcClass/getExcValue/createException when the
  // JPypeContext singleton that used to hold them (same package as this
  // class) was retired in favor of NativeContext.
  public long cls;
  public long value;

  public PyExceptionProxy(long l0, long l1)
  {
    cls = l0;
    value = l1;
  }

}
