// --- file: python/lang/SubInterpreterNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package python.lang;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import org.jpype.Script;
import org.jpype.SubInterpreter;
import org.jpype.internal.NativeLauncherControl;
import org.testng.SkipException;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNull;
import static org.testng.Assert.assertThrows;
import static org.testng.Assert.assertTrue;

/**
 * Exercises PEP 684 subinterpreter lifecycle: creation, GIL release back to
 * Java, basic execution, teardown, and that the main interpreter (already
 * running via {@link PyTestHarness}) remains unaffected.
 *
 * <p>
 * Subinterpreters require the native library to have been built against
 * Python 3.12+; on an older Python, {@code startSubInterpreter} throws
 * cleanly (see {@link #testUnsupportedVersionFailsCleanlyWithoutGilLeak}).
 * The rest of the tests skip themselves via {@link #startOrSkip} in that
 * case rather than failing, since there's nothing to exercise.</p>
 */
public class SubInterpreterNGTest extends PyTestHarness
{

  private static final String UNSUPPORTED_MESSAGE = "Subinterpreters not supported";

  private SubInterpreter startOrSkip()
  {
    SubInterpreter sub = new SubInterpreter();
    try
    {
      sub.start();
    } catch (RuntimeException ex)
    {
      if (ex.getMessage() != null && ex.getMessage().contains(UNSUPPORTED_MESSAGE))
        throw new SkipException("Subinterpreters require Python 3.12+ (native library built against an older Python)", ex);
      throw ex;
    }
    return sub;
  }

  @Test
  public void testUnsupportedVersionFailsCleanlyWithoutGilLeak()
  {
    SubInterpreter sub = new SubInterpreter();
    try
    {
      sub.start();
      // Supported: nothing more to check here, covered by the other tests.
      sub.close();
    } catch (RuntimeException ex)
    {
      assertTrue(ex.getMessage() != null && ex.getMessage().contains(UNSUPPORTED_MESSAGE), "Unexpected failure: " + ex);
    }
    // Whichever branch ran, the calling thread must not be left holding the GIL.
    assertFalse(NativeLauncherControl.isGilHeld(),
            "GIL leaked on the version-gate path");
  }

  @Test
  public void testStartAndStopReleasesGil()
  {
    SubInterpreter sub = startOrSkip();
    assertTrue(sub.isStarted());
    assertFalse(NativeLauncherControl.isGilHeld(),
            "GIL still held on calling thread immediately after SubInterpreter.start()");

    sub.close();
    assertFalse(NativeLauncherControl.isGilHeld(),
            "GIL still held on calling thread immediately after SubInterpreter.close()");
  }

  @Test
  public void testEvalInSubInterpreter()
  {
    SubInterpreter sub = startOrSkip();
    try
    {
      Script script = new Script(sub);
      assertEquals(script.eval("1 + 1").toString(), "2");
    } finally
    {
      sub.close();
    }
    assertFalse(NativeLauncherControl.isGilHeld());
  }

  @Test
  public void testMainInterpreterUnaffectedBySubInterpreter()
  {
    // Sanity check before: main interpreter's own long-lived Script (from
    // PyTestHarness) still works.
    assertEquals(context.eval("21 * 2").toString(), "42");

    SubInterpreter sub = startOrSkip();
    Script script = new Script(sub);
    assertEquals(script.eval("'sub' + 'interpreter'").toString(), "subinterpreter");
    sub.close();

    // Main interpreter must still be fully functional after a subinterpreter
    // was created and torn down alongside it.
    assertEquals(context.eval("21 * 2").toString(), "42");
    assertFalse(NativeLauncherControl.isGilHeld());
  }

  @Test
  public void testRepeatedStartStopDoesNotLeakGil()
  {
    for (int i = 0; i < 3; ++i)
    {
      SubInterpreter sub = startOrSkip();
      Script script = new Script(sub);
      assertEquals(script.eval("1").toString(), "1");
      sub.close();
      assertFalse(NativeLauncherControl.isGilHeld(),
              "GIL leaked on iteration " + i);
    }
  }

  @Test(timeOut = 15000)
  public void testMixAndMatchNoCrossTalkNoHangs()
  {
    // Interleave operations across the main interpreter and two independent
    // subinterpreters. Each has its own global "x" - if any of the three
    // shared state (module namespace clobbering, cached-object bleed
    // through the shared allocator/GIL, etc.) the wrong value would show up
    // after the interleaving. A hang here (deadlock between interpreters
    // sharing one process-wide GIL) fails via the method timeout rather
    // than wedging the whole test run.
    SubInterpreter sub1 = startOrSkip();
    SubInterpreter sub2 = startOrSkip();
    try
    {
      Script script1 = new Script(sub1);
      Script script2 = new Script(sub2);

      context.exec("x = 'main'");
      script1.exec("x = 'sub1'");
      script2.exec("x = 'sub2'");

      // Repeat with a different interleave order each round to catch any
      // ordering-dependent leakage between interpreters.
      for (int i = 0; i < 5; ++i)
      {
        assertEquals(context.eval("x").toString(), "main", "main clobbered on round " + i);
        assertEquals(script1.eval("x").toString(), "sub1", "sub1 clobbered on round " + i);
        assertEquals(script2.eval("x").toString(), "sub2", "sub2 clobbered on round " + i);

        script2.exec("x = 'sub2'");
        context.exec("x = 'main'");
        script1.exec("x = 'sub1'");
      }

      // Independent arithmetic, interleaved, to also exercise real
      // class-registration/proxy paths beyond simple string globals.
      assertEquals(context.eval("21 * 2").toString(), "42");
      assertEquals(script1.eval("1 + 1").toString(), "2");
      assertEquals(script2.eval("3 + 4").toString(), "7");
      assertEquals(context.eval("21 * 2").toString(), "42");
    } finally
    {
      sub1.close();
      sub2.close();
    }

    // Main must still be fully functional after both subs were created and
    // torn down alongside it.
    assertEquals(context.eval("21 * 2").toString(), "42");
  }

  @Test(timeOut = 15000)
  public void testConcurrentThreadDoesNotBlockAfterSubEval() throws InterruptedException
  {
    // isGilHeld() is known to report a spurious "still held" after
    // subinterpreter eval calls (see jpype-subinterpreter-difficulty
    // memory). This test distinguishes a real leak from a diagnostic false
    // positive directly: if the GIL were genuinely still held by this
    // thread, a *different* OS thread trying to acquire it for the main
    // interpreter (GIL is process-wide/shared in this subinterpreter model)
    // would block until this thread released it - i.e. it would still be
    // alive after the join timeout below. If it's just stale
    // PyGILState_Check() bookkeeping, the other thread proceeds immediately
    // regardless of what isGilHeld() reports here.
    SubInterpreter sub = startOrSkip();
    try
    {
      Script script = new Script(sub);
      assertEquals(script.eval("1 + 1").toString(), "2");
      // Intentionally not asserting on isGilHeld() here - it may or may not
      // spuriously report true; that's the exact claim under test below.

      AtomicReference<String> result = new AtomicReference<>();
      AtomicReference<Throwable> failure = new AtomicReference<>();
      Thread other = new Thread(() ->
      {
        try
        {
          result.set(context.eval("6 * 7").toString());
        } catch (Throwable t)
        {
          failure.set(t);
        }
      });
      other.start();
      other.join(5000);

      assertFalse(other.isAlive(),
              "Second thread is still blocked 5s after starting a main-interpreter "
              + "eval right after a subinterpreter eval - the GIL really is held "
              + "elsewhere; isGilHeld() was NOT a false positive.");
      assertNull(failure.get(), "Second thread failed: " + failure.get());
      assertEquals(result.get(), "42");
    } finally
    {
      sub.close();
    }
  }

  @Test(timeOut = 15000)
  public void testSmuggledProxyAcrossInterpretersThrows()
  {
    // "Smuggler" scenario (plan/Smuggler.md): sub1 creates a Python object,
    // it gets wrapped as a Java proxy (python.lang.PyObject) and dropped
    // into a plain Java container - no interpreter tagging on the Java
    // side. sub2 then pulls it back out. Handing that proxy's underlying
    // PyObject* straight back into sub2's Python code without any
    // interpreter-identity check would touch memory owned by sub1's
    // allocator/arena under sub2's GIL - real corruption under true
    // own-GIL isolation, not just a wrong answer. The minimum fix is to
    // detect the mismatch and fail loudly instead of silently corrupting.
    SubInterpreter sub1 = startOrSkip();
    SubInterpreter sub2 = startOrSkip();
    try
    {
      Script script1 = new Script(sub1);
      PyObject smuggled = script1.eval("object()");

      List<Object> container = new ArrayList<>();
      container.add(smuggled);

      Script bootstrap = new Script(sub2);
      java.util.Map<String, Object> vars = new java.util.HashMap<>();
      vars.put("container", container);
      PyDict globals2 = bootstrap.dictFromMap(vars);
      Script script2 = new Script(sub2, globals2, globals2);

      assertThrows(RuntimeException.class, () -> script2.eval("container.get(0)"));
    } finally
    {
      sub1.close();
      sub2.close();
    }
    assertFalse(NativeLauncherControl.isGilHeld(),
            "GIL leaked after the cross-interpreter smuggle attempt");
  }
}
