// --- file: org/jpype/ref/NativeReferenceQueue.java ---
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

import java.lang.ref.ReferenceQueue;
import java.util.logging.Logger;
import java.util.logging.Level;
import org.jpype.MainInterpreter;
import org.jpype.internal.NativeContext;

/**
 * A reference queue that binds the lifetime of Python objects to Java objects.
 *
 * <p>
 * This class is used internally by JPype to manage the lifecycle of Python
 * objects (`PyObject*`) that are referenced by Java objects. It ensures that
 * Python objects do not get garbage collected prematurely when their references
 * in Python fall to zero, as long as they are still referenced by Java.</p>
 *
 * <p>
 * The {@code JPypeReferenceQueue} maintains a thread that monitors the queue
 * for garbage-collected Java objects and performs cleanup operations on the
 * associated Python objects. It uses phantom references to track the lifecycle
 * of Java objects.</p>
 *
 * <p>
 * This class is a singleton, and its instance can be accessed via
 * {@link #getInstance()}.</p>
 *
 * <p>
 * Note: This class is intended for internal use and should not be used directly
 * by external code.</p>
 *
 * @author smenard
 */
public final class NativeReferenceQueue extends ReferenceQueue<Object>
{

  final static Logger LOGGER = Logger.getLogger(NativeReferenceQueue.class.getName());

  /**
   * A set of active references to Python objects.
   */
  private ReferenceSet hostReferences;

  /**
   * Indicates whether the reference queue has been stopped.
   */
  private boolean isStopped = false;

  /**
   * The thread responsible for monitoring the reference queue.
   */
  private Thread queueThread;

  /**
   * Mutex used to synchronize stopping the queue thread.
   */
  private final Object queueStopMutex = new Object();

  private final long address;

  /**
   * Sentinel reference used to wake up the queue thread periodically.
   */
  NativeReference sentinel;

  /**
   * Private constructor to initialize the reference queue.
   *
   * <p>
   * This constructor sets up the reference queue, initializes the native
   * bindings, and adds a sentinel reference.</p>
   */
  public NativeReferenceQueue(NativeContext context)
  {
    super();
    this.address = context.address();
    this.hostReferences = new ReferenceSet(address);
    addSentinel();
    NativeReference.removeHostReference(address, 0, 0);
    try
    {
      NativeReference.init(address, this, getClass().getDeclaredMethod("registerRef", Object.class, Long.TYPE, Long.TYPE));
    } catch (NoSuchMethodException | SecurityException ex)
    {
      throw new RuntimeException(ex);
    }
  }

  /**
   * Registers a reference to bind the lifetime of a Python object to a Java
   * object.
   *
   * <p>
   * This method adds an extra reference to a Python object (`PyObject*`) and
   * holds it until the Java object is garbage collected. When the Java object
   * is collected, the Python object is cleaned up.</p>
   *
   * @param javaObject The Java object to bind the lifetime to.
   * @param host The pointer to the Python object.
   * @param cleanup The pointer to the cleanup function for the Python object.
   */
  public void registerRef(Object javaObject, long host, long cleanup)
  {
    if (cleanup == 0)
    {
      return;
    }
    if (isStopped)
    {
      NativeReference.removeHostReference(address, host, cleanup);
    } else
    {
      NativeReference ref = new NativeReference(this, javaObject, host, cleanup);
      hostReferences.add(ref);
    }
  }

  /**
   * Starts the reference queue thread.
   *
   * <p>
   * This thread monitors the queue for garbage-collected Java objects and
   * performs cleanup operations on the associated Python objects.</p>
   */
  public void start()
  {
    isStopped = false;
    queueThread = new Thread(new Worker(), "Python Reference Queue");
    queueThread.setDaemon(true);
    queueThread.start();
    LOGGER.info("NativeReferenceQueue worker thread started.");
  }

  /**
   * Stops the reference queue thread.
   *
   * <p>
   * This method is called when the JVM shuts down to stop the reference queue
   * and perform any remaining cleanup operations.</p>
   */
  public void stop()
  {
    // Idempotency guard: this is now called both explicitly (from
    // MainInterpreter.close(), before Py_Finalize()) and later as a backstop
    // from the JVM-shutdown-hook path (NativeContext.shutdown()). Without
    // this check, the second call would interrupt an already-dead
    // queueThread and then block on queueStopMutex.wait(10000) with nobody
    // left to notifyAll() it - a spurious 10s hang.
    synchronized (this)
    {
      if (isStopped)
        return;
      isStopped = true;
    }

    LOGGER.info("NativeReferenceQueue worker thread shutting down.");
    try
    {
      synchronized (queueStopMutex)
      {
        queueThread.interrupt();

        // Wait for the thread to finish
        queueStopMutex.wait(10000);
      }
    } catch (InterruptedException ex)
    {
      // Ignore interruptions
      LOGGER.log(Level.WARNING, "Interrupted worker stop", ex);
    }

    // Empty the queue
    if (!MainInterpreter.getInstance().isJava())
    {
      hostReferences.flush();
    }
    LOGGER.info("NativeReferenceQueue worker thread stopped.");
  }

  /**
   * Checks whether the reference queue is running.
   *
   * @return {@code true} if the queue is running; {@code false} otherwise.
   */
  public boolean isRunning()
  {
    return !isStopped;
  }

  /**
   * Returns the number of items currently in the reference queue.
   *
   * @return The number of Python resources held by the reference queue.
   */
  public int getQueueSize()
  {
    return this.hostReferences.size();
  }

  /**
   * Adds a sentinel reference to the queue.
   *
   * <p>
   * The sentinel reference is used to periodically wake up the queue
   * thread.</p>
   */
  final void addSentinel()
  {
    sentinel = new NativeReference(this, new byte[0], 0, 0);
  }

  /**
   * Thread responsible for monitoring the reference queue and deleting
   * resources.
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
          // Check if a reference has been queued
          NativeReference ref = (NativeReference) remove(250);
          if (ref == sentinel)
          {
            addSentinel();
            NativeReference.wake(address);
            continue;
          }
          if (ref != null)
          {
            long hostRef = ref.hostReference;
            long cleanup = ref.cleanup;
            hostReferences.remove(ref);
            NativeReference.removeHostReference(address, hostRef, cleanup);
          }
        } catch (InterruptedException ex)
        {
          // Ignore interruptions
        } catch (Exception ex)
        {
          LOGGER.log(Level.SEVERE, "Critical error in ReferenceQueue worker", ex);
        }
      }

      synchronized (queueStopMutex)
      {
        queueStopMutex.notifyAll();
      }
    }
  }
}
