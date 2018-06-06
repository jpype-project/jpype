package jpype.ref;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

public class JPypeReference extends PhantomReference
{
  private long mHostReference;

  public JPypeReference(Object arg0, ReferenceQueue arg1)
  {
    super(arg0, arg1);
  }

  long getHostReference()
  {
    return mHostReference;
  }

  void setHostReference(long aHostReference)
  {
    mHostReference = aHostReference;
  }

  public int hashCode()
  {
    return (int) mHostReference;
  }

  public boolean equals(Object arg0)
  {
    if (!(arg0 instanceof JPypeReference))
    {
      return false;
    }
    
    return ((JPypeReference) arg0).mHostReference == mHostReference;
  }
}
