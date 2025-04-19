/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  
  See NOTICE file for details.
**************************************************************************** */
package org.jpype.ref;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

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
final public class JPypeReferenceQueue extends ReferenceQueue
{

  private final static JPypeReferenceQueue INSTANCE = new JPypeReferenceQueue();
  private JPypeReferenceSet hostReferences;
  private boolean isStopped = false;
  private Thread queueThread;
  private Object queueStopMutex = new Object();
  private PhantomReference sentinel = null;

  public static JPypeReferenceQueue getInstance()
  {
    return INSTANCE;
  }

  private JPypeReferenceQueue()
  {
    super();
    this.hostReferences = new JPypeReferenceSet();
    addSentinel();
    JPypeReferenceNative.removeHostReference(0, 0);
    try
    {
      JPypeReferenceNative.init(this, getClass().getDeclaredMethod("registerRef", Object.class, Long.TYPE, Long.TYPE));
    } catch (NoSuchMethodException | SecurityException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  /**
   * (internal) Binds the lifetime of a Python object to a Java object.
   * <p>
   * JPype adds an extra reference to a PyObject* and then calls this method to
   * hold that reference until the Java object is garbage collected.
   *
   * @param javaObject is the object to bind the lifespan to.
   * @param host is the pointer to the host object.
   * @param cleanup is the pointer to the function to call to delete the
   * resource.
   */
  public void registerRef(Object javaObject, long host, long cleanup)
  {
    if (cleanup == 0)
      return;
    if (isStopped)
    {
      JPypeReferenceNative.removeHostReference(host, cleanup);
    } else
    {
      JPypeReference ref = new JPypeReference(this, javaObject, host, cleanup);
      hostReferences.add(ref);
    }
  }

  /**
   * Start the threading queue.
   */
  public void start()
  {
    isStopped = false;
    queueThread = new Thread(new Worker(), "Python Reference Queue");
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
    try
    {
      synchronized (queueStopMutex)
      {
        synchronized (this)
        {
          isStopped = true;
          queueThread.interrupt();
        }

        // wait for the thread to finish ...
        queueStopMutex.wait(10000);
      }
    } catch (InterruptedException ex)
    {
      // who cares ...
    }

    // Empty the queue.
    hostReferences.flush();
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
          if (ref == sentinel)
          {
            addSentinel();
            JPypeReferenceNative.wake();
            continue;
          }
          if (ref != null)
          {
            long hostRef = ref.hostReference;
            long cleanup = ref.cleanup;
            hostReferences.remove(ref);
            JPypeReferenceNative.removeHostReference(hostRef, cleanup);
          }
        } catch (InterruptedException ex)
        {
          // don't know why ... don't really care ...
        }
      }

      synchronized (queueStopMutex)
      {
        queueStopMutex.notifyAll();
      }
    }
  }

  final void addSentinel()
  {
    sentinel = new JPypeReference(this, new byte[0], 0, 0);
  }

//</editor-fold>
}
