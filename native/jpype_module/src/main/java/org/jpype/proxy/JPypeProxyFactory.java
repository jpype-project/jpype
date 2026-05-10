package org.jpype.proxy;

import java.util.Arrays;
import java.util.Comparator;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class JPypeProxyFactory
{

  private final Map<InterfaceKey, JPypeProxyType> typeCache = new ConcurrentHashMap<>();

  // A reusable key per thread to avoid allocation during lookups
  private static final ThreadLocal<ReusableKey> LOOKUP_KEY
          = ThreadLocal.withInitial(ReusableKey::new);

  private static final JPypeProxyFactory INSTANCE = new JPypeProxyFactory();

  public static JPypeProxyType getProxyType(long cleanup, Class<?>[] interfaces)
  {
    return INSTANCE.getProxyTypeImpl(cleanup, interfaces);
  }

  JPypeProxyType getProxyTypeImpl(long cleanup, Class<?>[] interfaces)
  {
    // 1. Sort the input array in-place if allowed, or use a local stack-based sort
    // For now, we canonicalize. To truly avoid churn, the caller (C++) 
    // could provide them pre-sorted.
    Arrays.sort(interfaces, Comparator.comparing(Class::getName));

    // 2. Use the thread-local probe to check the map
    ReusableKey probe = LOOKUP_KEY.get().set(interfaces);
    JPypeProxyType existing = typeCache.get(probe);

    if (existing != null)
      return existing; // Found it! Zero objects allocated.

    // 3. Cache Miss: Only now do we pay the allocation tax
    Class<?>[] permanentArray = interfaces.clone();
    InterfaceKey permanentKey = new InterfaceKey(permanentArray);

    return typeCache.computeIfAbsent(permanentKey,
            k -> new JPypeProxyType(cleanup, permanentArray));
  }

  // Specialized key that doesn't own the array, just points to it
  private static class ReusableKey
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
    public int hashCode()
    {
      return hash;
    }

    @Override
    public boolean equals(Object obj)
    {
      // This works because InterfaceKey and ReusableKey 
      // both compare their internal arrays via Arrays.equals
      if (obj instanceof InterfaceKey)
        return Arrays.equals(this.ref, ((InterfaceKey) obj).interfaces);
      return false;
    }
  }

  private static class InterfaceKey
  {

    private final Class<?>[] interfaces;
    private final int hashCode;

    InterfaceKey(Class<?>[] interfaces)
    {
      this.interfaces = interfaces;
      this.hashCode = Arrays.hashCode(interfaces);
    }

    @Override
    public boolean equals(Object obj)
    {
      if (this == obj)
        return true;
      if (!(obj instanceof InterfaceKey))
        return false;
      return Arrays.equals(this.interfaces, ((InterfaceKey) obj).interfaces);
    }

    @Override
    public int hashCode()
    {
      return hashCode;
    }
  }
}
