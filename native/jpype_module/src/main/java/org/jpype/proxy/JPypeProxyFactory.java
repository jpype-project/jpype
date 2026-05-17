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

    boolean getConvert();

    Class<?>[] getInterfaces();
  }

  /**
   * This is the front end for deduplication of proxy type declarations.
   *
   * @param cleanup
   * @param interfaces
   * @param convert flag to indicate that this type should be automatically unwrapped on return to python.
   * @return
   */
  public static JPypeProxyType getProxyType(long cleanup, Class<?>[] interfaces, boolean convert)
  {
    return INSTANCE.getProxyTypeImpl(cleanup, interfaces, convert);
  }

  JPypeProxyType getProxyTypeImpl(long cleanup, Class<?>[] interfaces, boolean convert)
  {
    Arrays.sort(interfaces, Comparator.comparing(Class::getName));

    // 1. Thread-local probe (Zero allocation)
    ReusableKey probe = LOOKUP_KEY.get().set(interfaces, convert);

    JPypeProxyType existing = typeCache.get(probe);

    if (existing != null)
      return existing;

    // 2. Cache Miss (Allocation tax)
    Class<?>[] permanentArray = interfaces.clone();
    InterfaceKey permanentKey = new InterfaceKey(permanentArray, convert);

    JPypeProxyType out = typeCache.computeIfAbsent(permanentKey,
            k -> new JPypeProxyType(cleanup, permanentArray, convert));
    
    System.out.println(out + " " + convert + " "+ out.getConvert());
    return out;
  }

  private static class ReusableKey implements ProxyKey
  {

    private Class<?>[] ref;
    private int hash;
    private boolean convert;

    ReusableKey set(Class<?>[] interfaces, boolean convert)
    {
      this.ref = interfaces;
      this.hash = Arrays.hashCode(interfaces)+ Boolean.hashCode(convert);
      return this;
    }

    @Override
    public boolean getConvert()
    {
      return convert;
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
      if (this.convert != ((ProxyKey) obj).getConvert())
        return false;
      return Arrays.equals(this.ref, ((ProxyKey) obj).getInterfaces());
    }
  }

  private static class InterfaceKey implements ProxyKey
  {

    private final Class<?>[] interfaces;
    private final int hashCode;
    private final boolean convert;

    InterfaceKey(Class<?>[] interfaces, boolean convert)
    {
      this.interfaces = interfaces;
      this.hashCode = Arrays.hashCode(interfaces);
      this.convert = convert;
    }

    @Override
    public boolean getConvert()
    {
      return convert;
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
      if (this.convert != ((ProxyKey) obj).getConvert())
        return false;
      return Arrays.equals(this.interfaces, ((ProxyKey) obj).getInterfaces());
    }
  }
}
