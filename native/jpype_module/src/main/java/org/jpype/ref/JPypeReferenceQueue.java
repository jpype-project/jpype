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
import org.jpype.bridge.Interpreter;

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
public final class JPypeReferenceQueue extends ReferenceQueue<Object>
{

  /**
   * Singleton instance of the {@code JPypeReferenceQueue}.
   */
  private final static JPypeReferenceQueue INSTANCE = new JPypeReferenceQueue();

  /**
   * A set of active references to Python objects.
   */
  private JPypeReferenceSet hostReferences;

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

  /**
   * Sentinel reference used to wake up the queue thread periodically.
   */
  private PhantomReference<Object> sentinel = null;

  /**
   * Returns the singleton instance of the {@code JPypeReferenceQueue}.
   *
   * @return The singleton instance of the reference queue.
   */
  public static JPypeReferenceQueue getInstance()
  {
    return INSTANCE;
  }

  /**
   * Private constructor to initialize the reference queue.
   *
   * <p>
   * This constructor sets up the reference queue, initializes the native
   * bindings, and adds a sentinel reference.</p>
   */
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
      JPypeReferenceNative.removeHostReference(host, cleanup);
    } else
    {
      JPypeReference ref = new JPypeReference(this, javaObject, host, cleanup);
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
    try
    {
      synchronized (queueStopMutex)
      {
        synchronized (this)
        {
          isStopped = true;
          queueThread.interrupt();
        }

        // Wait for the thread to finish
        queueStopMutex.wait(10000);
      }
    } catch (InterruptedException ex)
    {
      // Ignore interruptions
    }

    // Empty the queue
    if (!Interpreter.getInstance().isJava())
    {
      hostReferences.flush();
    }
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
    sentinel = new JPypeReference(this, new byte[0], 0, 0);
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
          // Ignore interruptions
        }
      }

      synchronized (queueStopMutex)
      {
        queueStopMutex.notifyAll();
      }
    }
  }
}
