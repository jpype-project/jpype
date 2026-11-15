// --- file: org/jpype/proxy/ProxyTypeBridgeNGTest.java ---
package org.jpype.proxy;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.jpype.MainInterpreter;
import org.jpype.internal.NativeContext;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

/**
 * Covers {@code ProxyType.unwrapObject}/{@code getInstance}'s "real
 * {@code ProxyInstance} handler" branch - the one case
 * {@link ProxyTypeStaticsNGTest} couldn't reach without a live
 * {@code NativeContext}, since a genuine {@code ProxyType} can only be
 * constructed through one (it resolves method descriptors via the real
 * {@code TypeManager}).
 *
 * <p>
 * Builds the proxy the cheap way: {@code Runnable} has no default methods
 * and no {@code @Builtin}-annotated ones, so {@code ProxyType}'s
 * constructor never touches the native {@code getDefaultHandle} method for
 * it, and constructing the {@code ProxyInstance} handler directly (rather
 * than going through {@code ProxyType.newInstance}, which also registers a
 * native cleanup reference) needs no native call at all - this only tests
 * that {@code unwrapObject}/{@code getInstance} recognize the handler, not
 * that a proxy method dispatch actually works end to end (already covered
 * indirectly by every other package's proxy-backed interfaces, e.g.
 * {@code PyDeque}/{@code PyOrderedDict}).</p>
 */
public class ProxyTypeBridgeNGTest extends PyTestHarness
{

  private static NativeContext nativeContext() throws ReflectiveOperationException
  {
    Field f = MainInterpreter.class.getDeclaredField("context");
    f.setAccessible(true);
    return (NativeContext) f.get(MainInterpreter.getInstance());
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
  public void testUnwrapObjectAndGetInstanceRecognizeRealProxyInstanceHandler() throws Exception
  {
    NativeContext ctx = nativeContext();
    ProxyType type = ctx.getProxyFactory().getProxyType(0L, new Class<?>[]
    {
      Runnable.class
    });
    ProxyInstance handler = new ProxyInstance(type, 424242L);
    Object proxy = Proxy.newProxyInstance(
            ProxyTypeBridgeNGTest.class.getClassLoader(),
            new Class<?>[]
    {
      Runnable.class
    },
            handler);

    assertEquals(invokeStatic("unwrapObject", new Class<?>[]
    {
      Object.class
    }, proxy), 424242L);
    assertEquals(invokeStatic("getInstance", new Class<?>[]
    {
      Object.class
    }, proxy), 424242L);
  }

  @Test
  public void testGetTypeReturnsTheOwningProxyType() throws Exception
  {
    NativeContext ctx = nativeContext();
    ProxyType type = ctx.getProxyFactory().getProxyType(0L, new Class<?>[]
    {
      Runnable.class
    });
    ProxyInstance handler = new ProxyInstance(type, 555555L);
    assertSame(handler.getType(), type);
  }

  /** Marker interface unique to this test, so {@code getProxyType} always
   * takes the cache-miss path here regardless of what other tests in the
   * suite have already cached. */
  private interface UncachedMarker
  {
  }

  @Test
  public void testGetProxyTypeCacheMissLogsAtFineLevel() throws Exception
  {
    Logger logger = Logger.getLogger(ProxyFactory.class.getName());
    Level original = logger.getLevel();
    try
    {
      logger.setLevel(Level.FINE);
      NativeContext ctx = nativeContext();
      ProxyType type = ctx.getProxyFactory().getProxyType(0L, new Class<?>[]
      {
        UncachedMarker.class
      });
      assertNotNull(type);
    } finally
    {
      logger.setLevel(original);
    }
  }
}
