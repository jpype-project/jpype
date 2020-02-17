package org.jpype.ref;

import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Set;

/**
 * Reference queue holds the life of python objects to be as long as java items.
 * <p>
 * Any java class that holds a pointer into python needs to have a reference so
 * that python object does not go away if the references in python fall to zero.
 * JPype will add an extra reference to the object which is removed when the
 * python object dies.
 *
 * @author smenard
 *
 */
public class JPypeReferenceQueue extends ReferenceQueue
{

  static private JPypeReferenceQueue mInstance;
  private Set mHostReferences = new HashSet();
  private boolean mStopped = false;
  private Thread mQueueThread;
  private Object mQueueStopMutex = new Object();

  private JPypeReferenceQueue()
  {
    super();
  }

  /**
   * Get the reference queue.
   *
   * @return the singleton instance.
   */
  static public JPypeReferenceQueue getInstance()
  {
    if (mInstance == null)
      mInstance = new JPypeReferenceQueue();
    return mInstance;
  }

  /**
   * (internal) Binds the lifetime of C memory to a Java object.
   */
  public void registerRef(Object javaObject, long host, long cleanup)
  {
    JPypeReference ref = new JPypeReference(this, javaObject, host, cleanup);
    mHostReferences.add(ref);
  }

  /**
   * Start the threading queue.
   * <p>
   * This method is long running. It will return only if the queue gets stopped.
   * <p>
   */
  public void start()
  {
    mStopped = false;
    mQueueThread = new Thread(new Worker());
    mQueueThread.setDaemon(true);
    mQueueThread.start();
  }

  /**
   * Stops the reference queue.
   * <p>
   * This is called by jpype when the jvm shuts down.
   */
  public void stop()
  {
    try
    {
      // wait for the thread to finish ...
      synchronized (mQueueStopMutex)
      {
        mStopped = true;
        mQueueStopMutex.wait(5000);

        // FIXME what happens to any references that are outstanding after
        // the queue is stopped.  They will never be cleared so that means
        // they can never be collected.
      }
    } catch (InterruptedException ex)
    {
      // who cares ...
      return;
    }
  }

  /**
   * Checks the status of the reference queue.
   *
   * @return true is the queue is running.
   */
  public boolean isRunning()
  {
    return !mStopped;
  }

  /**
   * Get the number of items in the reference queue.
   *
   * @return the number of python resources held.
   */
  public int getQueueSize()
  {
    return this.mHostReferences.size();
  }

//<editor-fold desc="internal" defaultstate="collapsed">
  /**
   * Native hook to delete a native resource.
   *
   * @param host is the address of memory in C.
   * @param cleanup is the address the function to cleanup the memory.
   */
  private static native void removeHostReference(long host, long cleanup);

  /**
   * Thread to monitor the queue and delete resources.
   */
  private class Worker implements Runnable
  {

    @Override
    public void run()
    {
      while (!mStopped)
      {
        try
        {
          // Check if a ref has been queued. and check if the thread has been
          // stopped every 0.25 seconds
          JPypeReference ref = (JPypeReference) remove(250);
          if (ref != null)
          {
            synchronized (mHostReferences)
            {
              mHostReferences.remove(ref);
            }
            long hostRef = ref.mHostReference;
            long cleanup = ref.mCleanup;
            ref.mHostReference = 0;
            ref.mCleanup = 0;
            removeHostReference(hostRef, cleanup);
          }
        } catch (InterruptedException ex)
        {
          // don't know why ... don't really care ...
        }
      }
      mHostReferences = null;
      synchronized (mQueueStopMutex)
      {
        mQueueStopMutex.notifyAll();
      }
    }
  }
//</editor-fold>
}
