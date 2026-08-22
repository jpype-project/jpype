// --- file: org/jpype/internal/NativeContext.java ---
package org.jpype.internal;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Logger;
import java.util.logging.Level;
import org.jpype.MainInterpreter;
import org.jpype.Reflector;
import org.jpype.annotation.Exported;
import org.jpype.manager.StringManager;
import org.jpype.manager.TypeFactory;
import org.jpype.manager.TypeFactoryNative;
import org.jpype.manager.TypeManager;
import org.jpype.pkg.Package;
import org.jpype.pkg.PackageManager;
import org.jpype.proxy.ProxyFactory;
import org.jpype.ref.GlobalPool;
import org.jpype.ref.NativeReferenceQueue;
import python.lang.PyBuiltIn;

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
 */
public class NativeContext
{

  final static Logger LOGGER = Logger.getLogger(MainInterpreter.class.getName());
  public static final String VERSION = "1.7.2.dev0";
  private PyBuiltIn builtin;
  private long contextAddress;

  /**
   * Registry for SPI-provided "mini-backends" (e.g. {@code python.io.IO}),
   * scoped to this interpreter instance rather than kept as a JVM-wide
   * static — a second, independent interpreter running in the same JVM
   * must not see (or clobber) another interpreter's registrations.
   */
  private final Map<Class<?>, Object> backendRegistry = new ConcurrentHashMap<>();

  private final TypeFactory typeFactory;
  private final TypeManager typeManager;
  private final ProxyFactory proxyFactory;
  private final DynamicClassLoader classLoader;
  private final NativeReferenceQueue referenceQueue;
  private final StringManager stringManager;
  private final Signal signal;
  private final GlobalPool globalPool;

  private final AtomicInteger shutdownFlag = new AtomicInteger();
  private final List<Thread> shutdownHooks = new ArrayList<>();
  private final List<Runnable> postHooks = new ArrayList<>();
  public static boolean freeResources = true;
  public final Reflector reflector;

  public static Logger getLogger()
  {
    return LOGGER;
  }

  private NativeContext(long nativeContext, DynamicClassLoader loader)
  {
    this.contextAddress = nativeContext;
    this.globalPool = new GlobalPool(nativeContext);
    this.classLoader = DynamicClassLoader.install(loader);

    this.typeFactory = new TypeFactoryNative();
    this.typeManager = new TypeManager(this, typeFactory);
    this.proxyFactory = new ProxyFactory(this);
    this.stringManager = new StringManager(this);
    this.referenceQueue = new NativeReferenceQueue(this);
    this.signal = new Signal(this);

    PackageManager.setClassLoader(this.classLoader);

    try
    {
      reflector = (Reflector) Class.forName("org.jpype.internal.Reflector0", true, loader)
              .getConstructor()
              .newInstance();
    } catch (ClassNotFoundException | NoSuchMethodException | SecurityException
            | InstantiationException | IllegalAccessException
            | IllegalArgumentException | InvocationTargetException ex)
    {
      throw new RuntimeException("Unable to create reflector " + ex.getMessage(), ex);
    }
  }

  /**
   * Constructs the Java side of the context only - no {@link GlobalPool}
   * client (storeGlobal/retrieveGlobal) may be reachable from native code
   * until this call returns and the native side has captured the result
   * as {@code m_JavaContext}.
   *
   * <p>
   * {@link #initialize} (which populates primitive types and other
   * machinery that calls back into native and may need
   * {@code storeGlobal}) must not run until after that capture - hence it
   * is a deliberately separate call from native, not folded into this
   * one.</p>
   */
  @Exported
  @SuppressWarnings("unused")
  private static NativeContext create(long nativeContext, DynamicClassLoader loader, String nativeLib) throws Throwable
  {
    if (System.getProperty("java.util.logging.config.file") == null)
    {
      // Increase level to WARNING to prevent interpreter messages from leaking out
      java.util.logging.Logger.getLogger("org.jpype").setLevel(java.util.logging.Level.WARNING);
    }
    LOGGER.log(Level.INFO, "Initializing NativeContext for native address: 0x{0}", Long.toHexString(nativeContext));
    try
    {
      if (nativeLib != null)
      {
        LOGGER.log(Level.INFO, "Loading library {0}", nativeLib);
        System.load(nativeLib);
      }

      return new NativeContext(nativeContext, loader);
    } catch (Throwable ex)
    {
      ex.printStackTrace(System.err);
      throw ex;
    }
  }

  @Exported
  @SuppressWarnings("unused")
  private void start(boolean interrupt) throws Throwable
  {
    try
    {
      initialize(interrupt);
      scanExistingJars();
    } catch (Throwable ex)
    {
      ex.printStackTrace(System.err);
      throw ex;
    }
  }

  public long address()
  {
    return contextAddress;
  }

  /**
   * Stores a Java object in this interpreter's {@link GlobalPool} and
   * returns a handle for it.
   *
   * <p>
   * Called from native code (see {@code JPJavaFrame::storeGlobal}) in
   * place of a JNI {@code NewGlobalRef} - ordinary Java reachability from
   * the pool keeps {@code obj} alive instead.</p>
   *
   * @param obj The object to hold a reference to.
   * @return The handle, resolvable via {@link #retrieveGlobal} or
   * {@link GlobalPool#tryRelease}.
   */
  public long storeGlobal(Object obj)
  {
    return globalPool.add(obj);
  }

  /**
   * Resolves a handle from {@link #storeGlobal} back to the object it
   * names.
   *
   * <p>
   * Called from native code (see {@code JPJavaFrame::retrieveGlobal}); the
   * returned object becomes a JNI local reference automatically, as the
   * return value of this call.</p>
   *
   * @param handle A handle previously returned by {@link #storeGlobal}.
   * @return The object, or {@code null} if the handle is stale or
   * foreign.
   */
  public Object retrieveGlobal(long handle)
  {
    return globalPool.get(handle);
  }

  public void setBuiltIn(PyBuiltIn builtin)
  {
    this.builtin = builtin;
  }

  public PyBuiltIn getBuiltIn()
  {
    return this.builtin;
  }

  /**
   * Registers an SPI-provided mini-backend instance for this interpreter,
   * keyed by its Java interface (e.g. {@code python.io.IO.class}).
   *
   * @param <T> the mini-backend interface type.
   * @param iface the mini-backend interface.
   * @param instance the instance to register.
   */
  public <T> void registerBackend(Class<T> iface, T instance)
  {
    backendRegistry.put(iface, instance);
  }

  /**
   * Looks up a previously-registered SPI mini-backend for this interpreter.
   *
   * @param <T> the mini-backend interface type.
   * @param iface the mini-backend interface.
   * @return the registered instance.
   * @throws IllegalStateException if nothing is registered for {@code iface}.
   */
  public <T> T getBackend(Class<T> iface)
  {
    Object instance = backendRegistry.get(iface);
    if (instance == null)
      throw new IllegalStateException("No backend registered for " + iface.getName());
    return iface.cast(instance);
  }

  private void initialize(boolean interrupt)
  {
    LOGGER.info("Starting JPype subsystems...");
    // Okay everything is setup so lets give it a go.
    this.typeManager.init();
    this.proxyFactory.init();

    referenceQueue.start();
    if (!interrupt)
      signal.installHandlers();
    Runtime.getRuntime().addShutdownHook(new Thread(() -> shutdown()));
    LOGGER.info("JPype context initialized successfully.");
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
  @Exported
  @SuppressWarnings(
          {
            "CallToThreadYield", "SleepWhileInLoop"
          })
  private void shutdown()
  {
    LOGGER.info("JPype shutdown sequence initiated.");
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
      onShutdown();
      LOGGER.info("JPype shutdown completed successfully.");
      Thread.yield();

    } catch (Throwable th)
    {
      LOGGER.log(Level.SEVERE, "Error during JPype shutdown", th);
    }

    if (freeResources)
    {
      // Release all Python references
      try
      {
        referenceQueue.stop();
      } catch (Throwable th)
      {
      }

      // Drop this interpreter's GlobalPool - after this, any handle it
      // ever minted (held by native JPClass/JPMethod/... wrappers) fails
      // to resolve instead of returning a stale/foreign object.
      try
      {
        globalPool.close();
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
      run.run();
    try
    {
      classLoader.close();
    } catch (IOException ex)
    {
      // ignored
    }

  }

  private static native void onShutdown();

  public void addShutdownHook(Thread th)
  {
    LOGGER.fine("JPype shutdown hook installed.");
    this.shutdownHooks.add(th);
  }

  public boolean removeShutdownHook(Thread th)
  {
    LOGGER.fine("JPype shutdown hook removed.");
    if (this.shutdownHooks.contains(th))
    {
      this.shutdownHooks.remove(th);
      return true;
    } else
      return Runtime.getRuntime().removeShutdownHook(th);
  }

  public DynamicClassLoader getClassLoader()
  {
    return this.classLoader;
  }

  public StringManager getStringManager()
  {
    return this.stringManager;
  }

  public TypeFactory getTypeFactory()
  {
    return this.typeFactory;
  }

  public TypeManager getTypeManager()
  {
    return this.typeManager;
  }

  public NativeReferenceQueue getReferenceQueue()
  {
    return this.referenceQueue;
  }

  public ProxyFactory getProxyFactory()
  {
    return this.proxyFactory;
  }

  public boolean isShutdown()
  {
    return shutdownFlag.get() > 0;
  }

  public void _addPost(Runnable run)
  {
    this.postHooks.add(run);
  }

  /**
   * Clear the current interrupt.
   *
   * @param x is true if an exception should be thrown.
   * @throws InterruptedException
   */
  @Exported
  public void clearInterrupt(boolean x) throws InterruptedException
  {
    LOGGER.info("Clear interrupt");
    try
    {
      Thread th = Thread.currentThread();

      if (!x)
      {
        Signal.acknowledgePy(this.contextAddress);
      }

      // Check if the current worker thread has been hit with a Java Thread.interrupt()
      if (th.isInterrupted())
      {
        // Clear the flag in C++ for this isolated context block
        Signal.acknowledgePy(this.contextAddress);

        // Clear the standard interrupted status bit on the Java thread.
        // Thread.interrupted() reads and clears the bit automatically, which is 
        // cleaner and faster than forcing a Thread.sleep(1) context switch.
        Thread.interrupted();
      }
    } catch (Exception ex)
    {
      // Catch-all to insulate the call path
    }

    // If the check was meant to explicitly assert and propagate an interrupt,
    // synthesize the throw here if we detected it.
    if (x && Thread.currentThread().isInterrupted())
    {
      throw new InterruptedException("Java execution interrupted by Python context signal");
    }
  }

  @Exported
  public boolean isPackage(String s)
  {
    s = Keywords.safepkg(s);
    return PackageManager.isPackage(s);
  }

  @Exported
  public Package getPackage(String s)
  {
    LOGGER.log(Level.FINER, "Package request for {0}", s);
    s = Keywords.safepkg(s);
    if (!PackageManager.isPackage(s))
      return null;
    return new Package(s);
  }

  @Exported
  public void newWrapper(long l)
  {
    synchronized (this.typeFactory)
    {
      this.typeFactory.newWrapper(this.contextAddress, l);
    }
  }

  private void scanExistingJars()
  {
    String[] paths = System.getProperty("java.class.path").split(File.pathSeparator);
    for (String path : paths)
    {
      LOGGER.log(Level.FINER, "Scanning jar: {0}", path);
      this.classLoader.scanJar(Paths.get(path));
    }
  }

  /**
   * Convert a SAM into a Python hash code for callable lookups.
   *
   * This is bound to a subinterpreter instance through the context.
   *
   * @param cls
   * @return
   */
  @Exported
  public long getFunctional(Class<?> cls)
  {
    // Point this back to wherever your existing raw functional extraction method lives
    Method m = Functional.getFunctionalInterfaceMethod(cls);
    return m != null ? stringManager.get(m.getName()) : 0;
  }
}
