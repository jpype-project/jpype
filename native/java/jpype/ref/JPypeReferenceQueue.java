package jpype.ref;

import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Set;

/**
 * This class is a helper to manage the 2 different lifecyel systems.
 * 
 * @author smenard
 * 
 */
public class JPypeReferenceQueue extends ReferenceQueue implements Runnable
{
  /** also serves as mutex */
  private Set     mHostReferences = new HashSet();
  private boolean mStopped        = false;
  private Thread  mQueueThread;
  private Object  mQueueStopMutex = new Object();

  public void registerRef(JPypeReference ref, long hostRef)
  {
//    System.out.println("registering reference to "+hostRef);
    ref.setHostReference(hostRef);
    mHostReferences.add(ref);
  }

  /**
   * this method is long running. It will return only if the queue gets stopped
   * 
   */
  public void startManaging()
  {
    //System.out.println("Starting the reference queue thread");
    mQueueThread = new Thread(this);
    mQueueThread.setDaemon(true);
    mQueueThread.start();
  }

  public void run()
  {
    while (!mStopped)
    {
      try
      {
        // check if a ref has been queued. and check if the thred has been
        // stopped every 0.25 seconds
        JPypeReference ref = (JPypeReference) remove(250);
        if (ref != null)
        {
//          System.out.println("Got a reference! "+ref.getHostReference());
          synchronized (mHostReferences)
          {
            mHostReferences.remove(ref);
          }
          try
          {
            removeHostReference(ref.getHostReference());
          }
          finally
          {
            ref.setHostReference(-1);
          }
        }
      }
      catch (InterruptedException ex)
      {
        // don't know why ... don;t really care ...
      }
    }
    mHostReferences = null;
//    System.out.println("reference queue thread has stopped");
    synchronized (mQueueStopMutex)
    {
      mQueueStopMutex.notifyAll();
    }
  }

  public void stop()
  {
//    System.out.println("Stopping Reference queue");
    mStopped = true;
    try
    {
      // wait for the thread to finish ...
      synchronized (mQueueStopMutex)
      {
        mQueueStopMutex.wait(5000);
      }
    }
    catch (InterruptedException ex)
    {
      // who cares ...
      return;
    }
  }

  private static native void removeHostReference(long hostRef);
}
