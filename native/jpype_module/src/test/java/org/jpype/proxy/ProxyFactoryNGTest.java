// --- file: org/jpype/proxy/ProxyFactoryNGTest.java ---
package org.jpype.proxy;

import java.lang.reflect.Constructor;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Reflection-based coverage for {@code ProxyFactory}'s private cache-key
 * types ({@code ReusableKey}/{@code InterfaceKey}) - pure Java
 * {@code equals}/{@code hashCode} logic, no native/bridge dependency, but
 * both nested classes and their members are {@code private}.
 */
public class ProxyFactoryNGTest
{

  private static Object newInterfaceKey(Class<?>[] interfaces) throws ReflectiveOperationException
  {
    Class<?> keyClass = Class.forName("org.jpype.proxy.ProxyFactory$InterfaceKey");
    Constructor<?> ctor = keyClass.getDeclaredConstructor(Class[].class);
    ctor.setAccessible(true);
    return ctor.newInstance((Object) interfaces);
  }

  private static Object newReusableKey() throws ReflectiveOperationException
  {
    Class<?> keyClass = Class.forName("org.jpype.proxy.ProxyFactory$ReusableKey");
    Constructor<?> ctor = keyClass.getDeclaredConstructor();
    ctor.setAccessible(true);
    return ctor.newInstance();
  }

  private static Object setReusableKey(Object key, Class<?>[] interfaces) throws ReflectiveOperationException
  {
    java.lang.reflect.Method set = key.getClass().getDeclaredMethod("set", Class[].class);
    set.setAccessible(true);
    return set.invoke(key, (Object) interfaces);
  }

  @Test
  public void testInterfaceKeyEqualsSelfReturnsTrue() throws Exception
  {
    Object key = newInterfaceKey(new Class<?>[]
    {
      Runnable.class
    });
    assertTrue(key.equals(key));
  }

  @Test
  public void testInterfaceKeyEqualsFalseForNonProxyKey() throws Exception
  {
    Object key = newInterfaceKey(new Class<?>[]
    {
      Runnable.class
    });
    assertFalse(key.equals("not a key"));
  }

  @Test
  public void testInterfaceKeyEqualsTrueForSameInterfaceSet() throws Exception
  {
    Object a = newInterfaceKey(new Class<?>[]
    {
      Runnable.class
    });
    Object b = newInterfaceKey(new Class<?>[]
    {
      Runnable.class
    });
    assertTrue(a.equals(b));
    assertEquals(a.hashCode(), b.hashCode());
  }

  @Test
  public void testReusableKeyEqualsFalseForNonProxyKey() throws Exception
  {
    Object key = setReusableKey(newReusableKey(), new Class<?>[]
    {
      Runnable.class
    });
    assertFalse(key.equals("not a key"));
  }

  @Test
  public void testReusableKeyEqualsInterfaceKeyWithSameInterfaces() throws Exception
  {
    Object reusable = setReusableKey(newReusableKey(), new Class<?>[]
    {
      Runnable.class
    });
    Object interfaceKey = newInterfaceKey(new Class<?>[]
    {
      Runnable.class
    });
    assertTrue(reusable.equals(interfaceKey));
  }

  @Test
  public void testReusableKeyGetInterfacesReturnsSetArray() throws Exception
  {
    Class<?>[] interfaces = new Class<?>[]
    {
      Runnable.class
    };
    Object reusable = setReusableKey(newReusableKey(), interfaces);
    java.lang.reflect.Method getInterfaces = reusable.getClass().getMethod("getInterfaces");
    assertSame(getInterfaces.invoke(reusable), interfaces);
  }
}
