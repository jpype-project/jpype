// --- file: org/jpype/proxy/JPypeProxyFactory.java ---
package org.jpype.proxy;

import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class JPypeProxyFactory
{

  private JPypeProxyFactory()
  {
  }

  // Define a common interface for both keys
  // Map now uses the interface, silencing CodeQL
  private final Map<ProxyKey, JPypeProxyType> typeCache = new ConcurrentHashMap<>();
  private static final ThreadLocal<ReusableKey> LOOKUP_KEY
          = ThreadLocal.withInitial(ReusableKey::new);
  private static final JPypeProxyFactory INSTANCE = new JPypeProxyFactory();

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
  public static JPypeProxyType getProxyType(long cleanup, Class<?>[] interfaces)
  {
    return INSTANCE.getProxyTypeImpl(cleanup, interfaces);
  }

  JPypeProxyType getProxyTypeImpl(long cleanup, Class<?>[] interfaces)
  {
    Arrays.sort(interfaces, Comparator.comparing(Class::getName));

    // 1. Thread-local probe (Zero allocation)
    ReusableKey probe = LOOKUP_KEY.get().set(interfaces);

    JPypeProxyType existing = typeCache.get(probe);

    if (existing != null)
      return existing;

    // 2. Cache Miss (Allocation tax)
    Class<?>[] permanentArray = interfaces.clone();
    InterfaceKey permanentKey = new InterfaceKey(permanentArray);

    JPypeProxyType out = typeCache.computeIfAbsent(permanentKey,
            k -> new JPypeProxyType(cleanup, permanentArray));
    
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
}
