package jpype.ref;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

public class JPypeReference extends PhantomReference
{
  private long mHostReference;

  public JPypeReference(Object arg0, ReferenceQueue arg1, long host)
  {
    super(arg0, arg1);
    this.mHostReference = host;
  }

/*
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
*/

  public void dispose()
  {
     removeHostReference(mHostReference);
  }

  private static native void removeHostReference(long hostRef);
}
