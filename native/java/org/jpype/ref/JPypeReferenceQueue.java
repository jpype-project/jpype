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
  public long context = 0;
  private Set<JPypeReference> hostReferences = new HashSet();
  private boolean isStopped = false;
  private Thread queueThread;
  private Object queueStopMutex = new Object();

  public JPypeReferenceQueue()
  {
  }

  public JPypeReferenceQueue(long context)
  {
    super();
    this.context = context;
  }

  /**
   * (internal) Binds the lifetime of a Python object to a Java object.
   * <p>
   * JPype adds an extra reference to a PyObject* and then calls this method to
   * hold that reference until the Java object is garbage collected.
   *
   * @param javaObject
   * @param pythonObject
   */
  public void registerRef(Object javaObject, long pythonObject)
  {
    System.out.println("REGISTER REFERENCE");
    JPypeReference ref = new JPypeReference(this, javaObject, pythonObject);
    hostReferences.add(ref);
  }

  /**
   * Start the threading queue.
   */
  public void start()
  {
    System.out.println("Reference Queue start");
    isStopped = false;
    queueThread = new Thread(new Worker());
    queueThread.setDaemon(true);
    queueThread.start();
  }

  /**
   * Stops the reference queue.
   * <p>
   * This is called by jpype when the jvm shuts down.
   */
  public void stop()
  {
    System.out.println("Reference Queue stop");
    try
    {
      // wait for the thread to finish ...
      synchronized (queueStopMutex)
      {
        isStopped = true;
        queueStopMutex.wait(5000);

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
    return !isStopped;
  }

  /**
   * Get the number of items in the reference queue.
   *
   * @return the number of python resources held.
   */
  public int getQueueSize()
  {
    return this.hostReferences.size();
  }

//<editor-fold desc="internal" defaultstate="collapsed">
  /**
   * Native hook to delete a python resource.
   *
   * @param hostRef is the address of the python object (cast to PyObject*).
   */
  private static native void removeHostReference(long context, long hostRef);

  /**
   * Thread to monitor the queue and delete resources.
   */
  private class Worker implements Runnable
  {
    @Override
    public void run()
    {
      while (!isStopped)
      {
        try
        {
          // Check if a ref has been queued. and check if the thread has been
          // stopped every 0.25 seconds
          JPypeReference ref = (JPypeReference) remove(250);
          if (ref != null)
          {
            synchronized (hostReferences)
            {
              hostReferences.remove(ref);
            }
            long hostRef = ref.hostReference;
            ref.hostReference = -1;
            System.out.println("REMOVE REFERENCE" + hostRef);
            removeHostReference(context, hostRef);
          }
        } catch (InterruptedException ex)
        {
          // don't know why ... don't really care ...
        }
      }

      System.out.println("Kill remaining references");
      // We have references into Python which will never be freed if we don't 
      // remove them now
      for (JPypeReference ref : hostReferences)
      {
        removeHostReference(context, ref.hostReference);
      }
      System.out.println("Kill remaining references done");

      hostReferences = null;
      synchronized (queueStopMutex)
      {
        queueStopMutex.notifyAll();
      }
    }
  }
//</editor-fold>
}
