// --- file: org/jpype/internal/NativeContextNGTest.java ---
package org.jpype.internal;

import java.lang.reflect.Field;
import java.util.List;
import java.util.logging.Logger;
import org.jpype.MainInterpreter;
import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyTestHarness;

/**
 * Covers a handful of {@code NativeContext} members that are otherwise only
 * exercised indirectly.
 *
 * <p>
 * {@code NativeContext} has a private constructor and is only ever created
 * as part of standing up a live embedded interpreter (see
 * {@link python.lang.PyTestHarness}), so these tests reach it through the
 * already-running {@code context} field ({@code context.getContext()}),
 * exactly like {@code ProxyTypeBridgeNGTest} does.</p>
 */
public class NativeContextNGTest extends PyTestHarness
{

  @Test
  public void testGetLoggerReturnsMainInterpreterLogger()
  {
    Logger logger = NativeContext.getLogger();
    assertNotNull(logger);
    // See NativeContext: LOGGER = Logger.getLogger(MainInterpreter.class.getName())
    assertEquals(logger.getName(), MainInterpreter.class.getName());
    // Static accessor should always return the same singleton logger.
    assertSame(NativeContext.getLogger(), logger);
  }

  @Test
  public void testAddPostRegistersRunnableInPostHooks() throws Exception
  {
    NativeContext ctx = context.getContext();
    Runnable hook = () ->
    {
    };
    Field postHooksField = NativeContext.class.getDeclaredField("postHooks");
    postHooksField.setAccessible(true);
    @SuppressWarnings("unchecked")
    List<Runnable> postHooks = (List<Runnable>) postHooksField.get(ctx);

    int before = postHooks.size();
    ctx._addPost(hook);
    try
    {
      assertEquals(postHooks.size(), before + 1);
      assertSame(postHooks.get(postHooks.size() - 1), hook);
    } finally
    {
      // Don't leave a dangling reference to a throwaway lambda hanging off
      // the shared, suite-lifetime NativeContext.
      postHooks.remove(hook);
    }
  }

  @Test
  public void testClearInterruptWhenNotInterruptedDoesNotThrow() throws Exception
  {
    NativeContext ctx = context.getContext();
    assertFalse(Thread.currentThread().isInterrupted());
    // Neither x=false nor x=true should throw when there is nothing to clear.
    ctx.clearInterrupt(false);
    ctx.clearInterrupt(true);
    assertFalse(Thread.currentThread().isInterrupted());
  }

  @Test
  public void testClearInterruptClearsPendingInterruptFlag() throws Exception
  {
    NativeContext ctx = context.getContext();
    Thread.currentThread().interrupt();
    try
    {
      // The interrupted-thread branch always clears the bit itself (via
      // Thread.interrupted()) before the trailing throw-check runs, so this
      // never actually throws - it just leaves the thread un-interrupted.
      ctx.clearInterrupt(true);
    } finally
    {
      // Guard against a leaked interrupt bit poisoning later tests, in case
      // the above assumption ever changes.
      Thread.interrupted();
    }
    assertFalse(Thread.currentThread().isInterrupted());
  }
}
