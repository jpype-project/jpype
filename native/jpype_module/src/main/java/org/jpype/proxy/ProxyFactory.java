// --- file: org/jpype/proxy/ProxyFactory.java ---
package org.jpype.proxy;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Comparator;
import java.util.concurrent.ConcurrentHashMap;
import org.jpype.internal.NativeContext;
import org.jpype.manager.TypeManager;
import org.jpype.manager.StringManager;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;
import java.util.logging.Level;

public class ProxyFactory
{

  final static Logger LOGGER = Logger.getLogger(ProxyFactory.class.getName());
  final NativeContext context;

  // Define a common interface for both keys
  // Map now uses the interface, silencing CodeQL
  private final Map<ProxyKey, ProxyType> typeCache = new ConcurrentHashMap<>();
  private static final ThreadLocal<ReusableKey> LOOKUP_KEY
          = ThreadLocal.withInitial(ReusableKey::new);
  final Map<Method, MethodDescriptor> objectMethods = new HashMap<>();

  public ProxyFactory(NativeContext context)
  {
    this.context = context;
  }

  private interface ProxyKey
  {

    Class<?>[] getInterfaces();
  }

  /**
   * This is the front end for deduplication of proxy type declarations.
   *
   * @param cleanup
   * @param interfaces
   * @return
   */
  public ProxyType getProxyType(long cleanup, Class<?>[] interfaces)
  {
    Arrays.sort(interfaces, Comparator.comparing(Class::getName));

    // 1. Thread-local probe (Zero allocation)
    ReusableKey probe = LOOKUP_KEY.get().set(interfaces);

    ProxyType existing = typeCache.get(probe);

    if (existing != null)
      return existing;

    // Only log if FINE is enabled to protect performance
    if (LOGGER.isLoggable(Level.FINE))
    {
      LOGGER.log(Level.FINE, "Cache miss for proxy interfaces: {0}", Arrays.toString(interfaces));
    }

    // 2. Cache Miss (Allocation tax)
    Class<?>[] permanentArray = interfaces.clone();
    InterfaceKey permanentKey = new InterfaceKey(permanentArray);

    ProxyType out = typeCache.computeIfAbsent(permanentKey,
            k -> new ProxyType(this, cleanup, permanentArray));

    return out;
  }

  private static class ReusableKey implements ProxyKey
  {

    private Class<?>[] ref;
    private int hash;

    ReusableKey set(Class<?>[] interfaces)
    {
      this.ref = interfaces;
      this.hash = Arrays.hashCode(interfaces);
      return this;
    }

    @Override
    public Class<?>[] getInterfaces()
    {
      return ref;
    }

    @Override
    public int hashCode()
    {
      return hash;
    }

    @Override
    public boolean equals(Object obj)
    {
      if (!(obj instanceof ProxyKey))
        return false;
      return Arrays.equals(this.ref, ((ProxyKey) obj).getInterfaces());
    }
  }

  private static class InterfaceKey implements ProxyKey
  {

    private final Class<?>[] interfaces;
    private final int hashCode;

    InterfaceKey(Class<?>[] interfaces)
    {
      this.interfaces = interfaces;
      this.hashCode = Arrays.hashCode(interfaces);
    }

    @Override
    public Class<?>[] getInterfaces()
    {
      return interfaces;
    }

    @Override
    public int hashCode()
    {
      return hashCode;
    }

    @Override
    public boolean equals(Object obj)
    {
      if (this == obj)
        return true;
      if (!(obj instanceof ProxyKey))
        return false;
      return Arrays.equals(this.interfaces, ((ProxyKey) obj).getInterfaces());
    }
  }

  public void init()
  {
    LOGGER.info("Initializing ProxyFactory method cache.");
    TypeManager tm = context.getTypeManager();
    StringManager sm = context.getStringManager();
    synchronized (tm)
    {
      for (Method method : Object.class.getMethods())
      {
        long returnType = tm.findClass(method.getReturnType());
        Class<?>[] params = method.getParameterTypes();
        long[] paramTypes = new long[params.length];
        for (int i = 0; i < params.length; i++)
          paramTypes[i] = tm.findClass(params[i]);
        objectMethods.put(method, new MethodDescriptor(sm.get(method.getName()), returnType, paramTypes, null, false));
      }
    }
  }

}
