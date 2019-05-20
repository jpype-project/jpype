package org.jpype.ref;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

/**
 * (internal) Reference to a PyObject*.
 */
class JPypeReference extends PhantomReference
{
  long mHostReference;

  public JPypeReference(ReferenceQueue arg1, Object javaObject, long hostObject)
  {
    super(javaObject, arg1);
    mHostReference = hostObject;
  }

  @Override
  public int hashCode()
  {
    return (int) mHostReference;
  }

  @Override
  public boolean equals(Object arg0)
  {
    if (!(arg0 instanceof JPypeReference))
    {
      return false;
    }

    return ((JPypeReference) arg0).mHostReference == mHostReference;
  }
}
