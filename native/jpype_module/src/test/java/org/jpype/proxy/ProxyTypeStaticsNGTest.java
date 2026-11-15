// --- file: org/jpype/proxy/ProxyTypeStaticsNGTest.java ---
package org.jpype.proxy;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import static org.testng.Assert.*;
import org.testng.annotations.Test;

/**
 * Reflection-based coverage for {@code ProxyType}'s private static helpers -
 * pure Java logic with no native/bridge dependency, but not reachable from
 * outside the class without reflection since they're JNI export hooks
 * ({@code unwrapPythonException}/{@code getInstance}, marked
 * {@code @SuppressWarnings("unused")}) or a private nested key type
 * ({@code MethodKey}).
 */
public class ProxyTypeStaticsNGTest
{

  private static Object newMethodKey(Method m) throws ReflectiveOperationException
  {
    Class<?> mkClass = Class.forName("org.jpype.proxy.ProxyType$MethodKey");
    Constructor<?> ctor = mkClass.getDeclaredConstructor(Method.class);
    ctor.setAccessible(true);
    return ctor.newInstance(m);
  }

  @Test
  public void testMethodKeyEqualsFalseForNonMethodKey() throws Exception
  {
    Object key = newMethodKey(String.class.getMethod("length"));
    assertFalse(key.equals("not a key"));
  }

  @Test
  public void testMethodKeyEqualsTrueForSameSignature() throws Exception
  {
    Object a = newMethodKey(String.class.getMethod("length"));
    Object b = newMethodKey(String.class.getMethod("length"));
    assertTrue(a.equals(b));
    assertEquals(a.hashCode(), b.hashCode());
  }

  @Test
  public void testMethodKeyEqualsFalseForDifferentParamTypes() throws Exception
  {
    Object a = newMethodKey(String.class.getMethod("indexOf", String.class));
    Object b = newMethodKey(String.class.getMethod("indexOf", int.class));
    assertFalse(a.equals(b));
  }

  @Test
  public void testMethodKeyEqualsFalseForDifferentName() throws Exception
  {
    Object a = newMethodKey(String.class.getMethod("length"));
    Object b = newMethodKey(String.class.getMethod("trim"));
    assertFalse(a.equals(b));
  }

  /** Overloads with the right names but wrong arities/param types, so
   * {@code isObjectMethodSignature}'s name-matches-but-arity/type-mismatches
   * branches can be probed too. */
  private static final class Lookalikes
  {
    boolean equals()
    {
      return false;
    }

    boolean equals(int x)
    {
      return false;
    }

    int hashCode(int x)
    {
      return x;
    }

    String toString(int x)
    {
      return "" + x;
    }
  }

  private static boolean isObjectMethodSignature(Class<?> owner, String name, Class<?>... params)
          throws Exception
  {
    Method target = null;
    for (Method m : owner.getDeclaredMethods())
      if (m.getName().equals(name) && java.util.Arrays.equals(m.getParameterTypes(), params))
      {
        target = m;
        break;
      }
    assertNotNull(target, "no such method to probe with: " + name);
    Method probe = ProxyType.class.getDeclaredMethod("isObjectMethodSignature", Method.class);
    probe.setAccessible(true);
    return (boolean) probe.invoke(null, target);
  }

  private static boolean isObjectMethodSignature(String name, Class<?>... params) throws Exception
  {
    return isObjectMethodSignature(String.class, name, params);
  }

  @Test
  public void testIsObjectMethodSignatureRecognizesEqualsHashCodeToString() throws Exception
  {
    assertTrue(isObjectMethodSignature("equals", Object.class));
    assertTrue(isObjectMethodSignature("hashCode"));
    assertTrue(isObjectMethodSignature("toString"));
  }

  @Test
  public void testIsObjectMethodSignatureRejectsUnrelatedMethod() throws Exception
  {
    assertFalse(isObjectMethodSignature("length"));
  }

  @Test
  public void testIsObjectMethodSignatureRejectsNameMatchWithWrongSignature() throws Exception
  {
    assertFalse(isObjectMethodSignature(Lookalikes.class, "equals"));
    assertFalse(isObjectMethodSignature(Lookalikes.class, "equals", int.class));
    assertFalse(isObjectMethodSignature(Lookalikes.class, "hashCode", int.class));
    assertFalse(isObjectMethodSignature(Lookalikes.class, "toString", int.class));
  }

  private static Object invokeStatic(String name, Class<?>[] paramTypes, Object... args)
          throws ReflectiveOperationException
  {
    Method m = ProxyType.class.getDeclaredMethod(name, paramTypes);
    m.setAccessible(true);
    try
    {
      return m.invoke(null, args);
    } catch (InvocationTargetException ex)
    {
      throw (RuntimeException) ex.getCause();
    }
  }

  @Test
  public void testUnwrapPythonExceptionNullReturnsZero() throws Exception
  {
    assertEquals(invokeStatic("unwrapPythonException", new Class<?>[]
    {
      Throwable.class
    }, (Object) null), 0L);
  }

  @Test
  public void testUnwrapPythonExceptionUnrelatedThrowableReturnsZero() throws Exception
  {
    assertEquals(invokeStatic("unwrapPythonException", new Class<?>[]
    {
      Throwable.class
    }, new RuntimeException("plain")), 0L);
  }

  @Test
  public void testUnwrapObjectNonProxyReturnsZero() throws Exception
  {
    assertEquals(invokeStatic("unwrapObject", new Class<?>[]
    {
      Object.class
    }, "not a proxy"), 0L);
  }

  @Test
  public void testUnwrapObjectProxyWithForeignHandlerReturnsZero() throws Exception
  {
    InvocationHandler handler = (proxy, method, args) -> null;
    Object proxy = Proxy.newProxyInstance(
            ProxyTypeStaticsNGTest.class.getClassLoader(),
            new Class<?>[]
    {
      Runnable.class
    },
            handler);
    assertEquals(invokeStatic("unwrapObject", new Class<?>[]
    {
      Object.class
    }, proxy), 0L);
  }

  @Test
  public void testGetInstanceNullReturnsZero() throws Exception
  {
    assertEquals(invokeStatic("getInstance", new Class<?>[]
    {
      Object.class
    }, (Object) null), 0L);
  }

  @Test
  public void testGetInstanceNonProxyReturnsZero() throws Exception
  {
    assertEquals(invokeStatic("getInstance", new Class<?>[]
    {
      Object.class
    }, "not a proxy"), 0L);
  }

  @Test
  public void testGetInstanceProxyWithForeignHandlerReturnsZero() throws Exception
  {
    InvocationHandler handler = (proxy, method, args) -> null;
    Object proxy = Proxy.newProxyInstance(
            ProxyTypeStaticsNGTest.class.getClassLoader(),
            new Class<?>[]
    {
      Runnable.class
    },
            handler);
    assertEquals(invokeStatic("getInstance", new Class<?>[]
    {
      Object.class
    }, proxy), 0L);
  }
}
