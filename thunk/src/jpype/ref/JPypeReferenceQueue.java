package jpype.ref;

import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Set;

/**
 * This class is a helper to manage the 2 different lifecycle systems.
 * 
 * @author smenard
 * 
 */
public class JPypeReferenceQueue extends ReferenceQueue implements Runnable
{
  /** also serves as mutex */
  private boolean mStopped = false;
  private Thread  mQueueThread;
  private Object  mQueueStopMutex = new Object();
  private int references;

  /** Create a new managed referecen between java and the host.
   */
  public void registerRef(Object obj, long hostRef)
  {
    // Create a new reference
    JPypeReference ref = new JPypeReference(obj, this, hostRef);
    references++;
  }

  /**
   * this method is long running. It will return only if the queue gets stopped
   * 
   */
  public void start()
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
        // check if a ref has been queued. and check if the thread has been
        // stopped every 0.25 seconds
        JPypeReference ref = (JPypeReference) remove(250);
        if (ref != null)
        {
          ref.dispose();
          references--;
        }
      }
      catch (InterruptedException ex)
      {
        // don't know why ... don't really care ...
      }
    }
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

  public int getReferenceCount()
  {
    return references;
  }

}
