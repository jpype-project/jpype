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
package org.jpype;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.Buffer;
import java.nio.ByteOrder;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import org.jpype.manager.TypeFactory;
import org.jpype.manager.TypeFactoryNative;
import org.jpype.manager.TypeManager;
import org.jpype.pkg.JPypePackage;
import org.jpype.pkg.JPypePackageManager;
import org.jpype.ref.JPypeReferenceQueue;

/**
 * Context for JPype.
 * <p>
 * This is the part of JPype that holds all resources. After the classloader is
 * created this class is given the address of the context object in JPype. Any
 * resources in JPype Java layer can be contacted using the context.
 * <p>
 * Boot order is - create the C++ portion of the context. - start the jvm - load
 * the bootloader - install the jar into the bootloader - install all native
 * methods using the bootloader - create the Java portion of the context. - use
 * the Java context to access the resources (ReferenceQueue, TypeFactory,
 * TypeManager)
 * <p>
 * Once started, python calls use the context to get a frame and attach their
 * threads. Methods called from Java will get the env and use it to get their
 * context from which they can create a frame.
 * <p>
 * The C++ context will hold all the previous global variables thus allowing the
 * C++ portion to be cleaned up properly when the JVM is shutdown or
 * disconnected.
 * <p>
 * As the JPypeContext can't be tested directly from Java code, it will need to
 * be kept light.
 * <p>
 * Our goal is to remove as much direct contact methods as possible from the C++
 * layer. Previous globals in JPTypeManager move to the context as do the
 * contents of JPJni.
 *
 *
 *
 * @author nelson85
 */
public class JPypeContext
{

  public final String VERSION = "1.7.2.dev0";

  private static final JPypeContext INSTANCE = new JPypeContext();
  // This is the C++ portion of the context.
  private long context;
  private TypeFactory typeFactory;
  private TypeManager typeManager;
  private JPypeClassLoader classLoader;
  private final AtomicInteger shutdownFlag = new AtomicInteger();
  private final List<Thread> shutdownHooks = new ArrayList<>();
  private final List<Runnable> postHooks = new ArrayList<>();
  public static boolean freeResources = true;
  public JPypeReflector reflector = null;

  static public JPypeContext getInstance()
  {
    return INSTANCE;
  }

  /**
   * Start the JPype system.
   *
   * @param context is the C++ portion of the context.
   * @param loader is the classloader holding JPype resources.
   * @return the created context.
   */
  private static JPypeContext createContext(long context, ClassLoader loader, String nativeLib, boolean interrupt) throws Throwable
  {
    try
    {
      if (nativeLib != null)
      {
        System.load(nativeLib);
      }
      INSTANCE.context = context;
      INSTANCE.classLoader = (JPypeClassLoader) loader;
      INSTANCE.typeFactory = new TypeFactoryNative();
      INSTANCE.typeManager = new TypeManager(context, INSTANCE.typeFactory);
      INSTANCE.initialize(interrupt);

      try
      {
        INSTANCE.reflector = (JPypeReflector) Class.forName("org.jpype.Reflector0", true, loader)
                .getConstructor()
                .newInstance();
      } catch (ClassNotFoundException | NoSuchMethodException | SecurityException
              | InstantiationException | IllegalAccessException
              | IllegalArgumentException | InvocationTargetException ex)
      {
        throw new RuntimeException("Unable to create reflector " + ex.getMessage(), ex);
      }

      scanExistingJars();
      return INSTANCE;
    } catch (Throwable ex)
    {
      ex.printStackTrace(System.err);
      throw ex;
    }
  }

  private JPypeContext()
  {
  }

  private void initialize(boolean interrupt)
  {
    // Okay everything is setup so lets give it a go.
    this.typeManager.init();
    JPypeReferenceQueue.getInstance().start();
    if (!interrupt)
      JPypeSignal.installHandlers();

    // Install a shutdown hook to clean up Python resources.
    Runtime.getRuntime().addShutdownHook(new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        INSTANCE.shutdown();
      }
    }));

  }

  /**
   * Shutdown and remove all Python resources.
   *
   * This hook is only called after the last user thread has died. Thus the only
   * remaining connections are proxies that were attached to the JVM shutdown
   * hook, the reference queue, and the typemanager.
   *
   * This routine will try to take out the last connections in an orderly
   * fashion. Inherently this is a very dangerous time as portions of Java have
   * already been deactivated.
   */
  @SuppressWarnings(
          {
            "CallToThreadYield", "SleepWhileInLoop"
          })
  private void shutdown()
  {
    try
    {
      // Try to yield in case there is a race condition.  The user
      // may have installed a shutdown hook, but we cannot verify
      // the order that shutdown hook threads are executed.  Thus we will
      // try to intentionally lose the race.
      //
      // This will only occur if something registered a shutdown hook through
      // a Java API.  Those registered though the JPype API will be joined
      // manually.
      for (int i = 0; i < 5; i++)
      {
        try
        {
          Thread.sleep(1);
          Thread.yield();
        } catch (InterruptedException ex)
        {
        }
      }

      // Execute any used defined shutdown hooks registered with JPype.
      if (!this.shutdownHooks.isEmpty())
      {
        for (Thread thread : this.shutdownHooks)
        {
          thread.start();
        }
        for (Thread thread : this.shutdownHooks)
        {
          try
          {
            thread.join();
          } catch (InterruptedException ex)
          {
          }
        }
      }

      // Disable all future calls to proxies
      this.shutdownFlag.incrementAndGet();

      // Past this point any further execution of a Python proxy would
      // be fatal.
      Thread t1 = Thread.currentThread();
      Map<Thread, StackTraceElement[]> threads = Thread.getAllStackTraces();

      for (Thread t : threads.keySet())
      {
        if (t1 == t || t.isDaemon())
          continue;
        t.interrupt();
      }

      // Inform Python no more calls are permitted
      onShutdown(this.context);
      Thread.yield();

    } catch (Throwable th)
    {
    }

    if (freeResources)
    {
      // Release all Python references
      try
      {
        JPypeReferenceQueue.getInstance().stop();
      } catch (Throwable th)
      {
      }

      // Release any C++ resources
      try
      {
        this.typeManager.shutdown();
      } catch (Throwable th)
      {
      }
    }

    // Execute post hooks
    for (Runnable run : this.postHooks)
    {
      run.run();
    }
    try
    {
      classLoader.close();
    } catch (IOException ex)
    {
      // ignored
    }

  }

  private static native void onShutdown(long ctxt);

  public void addShutdownHook(Thread th)
  {
    this.shutdownHooks.add(th);
  }

  public boolean removeShutdownHook(Thread th)
  {
    if (this.shutdownHooks.contains(th))
    {
      this.shutdownHooks.remove(th);
      return true;
    } else
      return Runtime.getRuntime().removeShutdownHook(th);
  }

  /**
   * Get the C++ portion.
   *
   * @return
   */
  public long getContext()
  {
    return context;
  }

  public ClassLoader getClassLoader()
  {
    if (this.classLoader == null)
      this.classLoader = new JPypeClassLoader(ClassLoader.getSystemClassLoader());
    return this.classLoader;
  }

  public TypeFactory getTypeFactory()
  {
    return this.typeFactory;
  }

  public TypeManager getTypeManager()
  {
    return this.typeManager;
  }

  /**
   * Add a hook to run after Python interface is shutdown.
   *
   * This must never have a Python method attached.
   *
   * @param run
   */
  public void _addPost(Runnable run)
  {
    this.postHooks.add(run);
  }

  public boolean isShutdown()
  {
    return shutdownFlag.get() > 0;
  }

  /**
   * Clear the current interrupt.
   *
   * @param x is true if an exception should be thrown.
   * @throws InterruptedException
   */
  public static void clearInterrupt(boolean x) throws InterruptedException
  {
    try
    {
      Thread th = Thread.currentThread();

      // Only relevant if this is the main thread for signal handling
      if (th != JPypeSignal.main)
        return;

      // Unconditionally clear the interrupt flag if we are called from 
      // C++.  This happens when a field get() or method call() is 
      // invoked.
      if (!x)
        JPypeSignal.acknowledgePy();

      // Check if this thread is interrupted
      if (th.isInterrupted())
      {
        // Clear the flag in C++
        JPypeSignal.acknowledgePy();

        // Clear the flag in Java
        Thread.sleep(1);
      }
    } catch (InterruptedException ex)
    {
      if (x)
        throw ex;
    }
  }

  public long getExcClass(Throwable th)
  {
    if (th instanceof PyExceptionProxy)
      return ((PyExceptionProxy) th).cls;
    return 0;
  }

  public long getExcValue(Throwable th)
  {
    if (th instanceof PyExceptionProxy)
      return ((PyExceptionProxy) th).value;
    return 0;
  }

  private Exception createException(long l0, long l1)
  {
    return new PyExceptionProxy(l0, l1);
  }

  private boolean order(Buffer b)
  {
    if (b instanceof java.nio.ByteBuffer)
      return ((java.nio.ByteBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.ShortBuffer)
      return ((java.nio.ShortBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.CharBuffer)
      return ((java.nio.CharBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.IntBuffer)
      return ((java.nio.IntBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.LongBuffer)
      return ((java.nio.LongBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.FloatBuffer)
      return ((java.nio.FloatBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    if (b instanceof java.nio.DoubleBuffer)
      return ((java.nio.DoubleBuffer) b).order() == ByteOrder.LITTLE_ENDIAN;
    return true;
  }

  public boolean isPackage(String s)
  {
    s = JPypeKeywords.safepkg(s);
    return JPypePackageManager.isPackage(s);
  }

  public JPypePackage getPackage(String s)
  {
    s = JPypeKeywords.safepkg(s);
    if (!JPypePackageManager.isPackage(s))
      return null;
    return new JPypePackage(s);
  }

  /**
   * Utility to probe functional interfaces.
   *
   * @param cls
   * @return
   */
  public static String getFunctional(Class cls)
  {
    Method m = JPypeUtilities.getFunctionalInterfaceMethod(cls);
    return m != null ? m.getName() : null;
  }

  /**
   * Utility function for extracting the unique portion of a stack trace.
   *
   * This is a bit different that the Java method which works from the back. We
   * will be using fake stacktraces from Python at some point so finding the
   * first common is a better approach.
   *
   * @param th is the throwable.
   * @param enclosing is the throwsble that holds this or null if top level.
   * @return the unique frames as an object array with 4 objects per frame.
   */
  public Object[] getStackTrace(Throwable th, Throwable enclosing)
  {
    StackTraceElement[] trace = th.getStackTrace();
    if (trace == null || enclosing == null)
      return toFrames(trace);
    StackTraceElement[] te = enclosing.getStackTrace();
    if (te == null)
      return toFrames(trace);
    for (int i = 0; i < trace.length; ++i)
    {
      if (trace[i].equals(te[0]))
      {
        return toFrames(Arrays.copyOfRange(trace, 0, i));
      }
    }
    return toFrames(trace);
  }

  private Object[] toFrames(StackTraceElement[] stackTrace)
  {
    if (stackTrace == null)
      return null;
    Object[] out = new Object[4 * stackTrace.length];
    int i = 0;
    for (StackTraceElement fr : stackTrace)
    {
      out[i++] = fr.getClassName();
      out[i++] = fr.getMethodName();
      out[i++] = fr.getFileName();
      out[i++] = fr.getLineNumber();
    }
    return out;

  }

  public void newWrapper(long l)
  {
    // We can only go through this point single file.
    synchronized (this.typeFactory)
    {
      this.typeFactory.newWrapper(context, l);
    }
  }

  private static void scanExistingJars()
  {
    // Scan existing jars for missing directory entries
    String[] paths = System.getProperty("java.class.path").split(File.pathSeparator);
    for (String path : paths)
    {
      INSTANCE.classLoader.scanJar(Paths.get(path));
    }
  }

  private static long getTotalMemory()
  {
    return Runtime.getRuntime().totalMemory();
  }

  private static long getFreeMemory()
  {
    return Runtime.getRuntime().freeMemory();
  }

  private static long getMaxMemory()
  {
    return Runtime.getRuntime().maxMemory();
  }

  private static long getUsedMemory()
  {
    return Runtime.getRuntime().totalMemory() - Runtime.getRuntime().freeMemory();
  }

  private static long getHeapMemory()
  {
    java.lang.management.MemoryMXBean memoryBean = java.lang.management.ManagementFactory.getMemoryMXBean();
    return memoryBean.getHeapMemoryUsage().getUsed();
  }
}
