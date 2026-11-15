// --- file: python/lang/SubInterpreterBuilderNGTest.java ---
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

import java.io.ByteArrayOutputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.util.function.Supplier;
import org.jpype.Script;
import org.jpype.SubInterpreter;
import org.jpype.SubInterpreterBuilder;
import org.jpype.internal.NativeLauncherControl;
import org.testng.SkipException;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNotSame;
import static org.testng.Assert.assertSame;

/**
 * Exercises {@link SubInterpreterBuilder}: the {@code Option} EnumSet
 * flags, the {@code ownGil()}/{@code elevated()} presets, cross-field
 * validation, stdio wiring, and the {@code asSupplier()} adapter.
 *
 * <p>
 * Same version-gating pattern as {@link SubInterpreterNGTest}: skip rather
 * than fail on a native library built against Python &lt; 3.12.</p>
 */
public class SubInterpreterBuilderNGTest extends PyTestHarness
{

  private static final String UNSUPPORTED_MESSAGE = "Subinterpreters not supported";

  private SubInterpreter startOrSkip(SubInterpreterBuilder builder)
  {
    try
    {
      return builder.start();
    } catch (RuntimeException ex)
    {
      if (ex.getMessage() != null && ex.getMessage().contains(UNSUPPORTED_MESSAGE))
        throw new SkipException("Subinterpreters require Python 3.12+ (native library built against an older Python)", ex);
      throw ex;
    }
  }

  @Test
  public void testElevatedMatchesPlainStart()
  {
    // elevated() reproduces the fixed shared-GIL/shared-obmalloc values
    // SubInterpreter.start() has always used.
    SubInterpreter sub = startOrSkip(SubInterpreterBuilder.elevated());
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
  public void testDefaultIsSafestNotElevated()
  {
    // A bare `new SubInterpreterBuilder()` must default to the safest legal
    // combination (own GIL, own obmalloc, check_multi_interp_extensions),
    // not the elevated/shared one - this API never shipped, so there is no
    // prior behavior to default to for backward compatibility.
    SubInterpreter sub = startOrSkip(new SubInterpreterBuilder());
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
  public void testOwnGilLaunchesAndImportsJpype()
  {
    // The whole point of ownGil(): a genuinely isolated own-GIL, own-obmalloc
    // subinterpreter that can still "import _jpype" (reachable now that
    // _jpype is multi-phase-init-safe - plan/MultiPhaseInit.md).
    SubInterpreter sub = startOrSkip(SubInterpreterBuilder.ownGil());
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
  public void testIllegalCombinationRejectedBeforeNativeCall()
  {
    // The default already has USE_MAIN_OBMALLOC off (it's the safest
    // combination - see SubInterpreterBuilder's class Javadoc), so the
    // illegal combination requires explicitly disabling
    // CHECK_MULTI_INTERP_EXTENSIONS too, not just USE_MAIN_OBMALLOC.
    SubInterpreterBuilder builder = new SubInterpreterBuilder()
            .without(SubInterpreterBuilder.Option.CHECK_MULTI_INTERP_EXTENSIONS);
    try
    {
      builder.start();
      throw new AssertionError("Expected IllegalStateException, but start() succeeded");
    } catch (IllegalStateException expected)
    {
      // pass
    }
    assertFalse(NativeLauncherControl.isGilHeld(),
            "GIL leaked on the validation-rejection path (native call should never have run)");
  }

  @Test
  public void testSetOutputCapturesPrint()
  {
    ByteArrayOutputStream captured = new ByteArrayOutputStream();
    SubInterpreterBuilder builder = SubInterpreterBuilder.elevated().setOutput(captured);
    SubInterpreter sub = startOrSkip(builder);
    try
    {
      Script script = new Script(sub);
      script.exec("print('hello from builder', end='')");
      script.exec("import sys; sys.stdout.flush()");
      assertEquals(new String(captured.toByteArray(), StandardCharsets.UTF_8), "hello from builder");
    } finally
    {
      sub.close();
    }
  }

  @Test
  public void testAsSupplierLaunchesIndependentInstances()
  {
    SubInterpreterBuilder builder = SubInterpreterBuilder.elevated();
    Supplier<SubInterpreter> supplier = builder.asSupplier();

    SubInterpreter sub1;
    try
    {
      sub1 = supplier.get();
    } catch (RuntimeException ex)
    {
      if (ex.getMessage() != null && ex.getMessage().contains(UNSUPPORTED_MESSAGE))
        throw new SkipException("Subinterpreters require Python 3.12+ (native library built against an older Python)", ex);
      throw ex;
    }
    SubInterpreter sub2 = supplier.get();
    try
    {
      assertNotSame(sub1, sub2);
      Script script1 = new Script(sub1);
      Script script2 = new Script(sub2);
      script1.exec("x = 'one'");
      script2.exec("x = 'two'");
      assertEquals(script1.eval("x").toString(), "one");
      assertEquals(script2.eval("x").toString(), "two");
    } finally
    {
      sub1.close();
      sub2.close();
    }
  }

  @Test
  public void testSetterOverloadsChainAndAssign()
  {
    // These setters are plain field assignments returning `this` for
    // chaining - no native launch needed, so unlike the rest of this class
    // they don't need startOrSkip()/version gating. Only the OutputStream
    // overload of setOutput and the wiring inside start() (env-gated, see
    // testSetOutputCapturesPrint) were previously exercised; this covers
    // the remaining Writer/Reader/InputStream overloads.
    SubInterpreterBuilder builder = new SubInterpreterBuilder();
    assertSame(builder.setOutput(new OutputStreamWriter(new ByteArrayOutputStream())), builder);
    assertSame(builder.setError(new ByteArrayOutputStream()), builder);
    assertSame(builder.setError(new OutputStreamWriter(new ByteArrayOutputStream())), builder);
    assertSame(builder.setInput(new java.io.ByteArrayInputStream(new byte[0])), builder);
    assertSame(builder.setInput(new StringReader("")), builder);
  }

  @Test
  public void testStartWiresWriterOutputErrorAndReaderInput()
  {
    // testSetOutputCapturesPrint/testSetterOverloadsChainAndAssign already
    // cover the OutputStream/InputStream overloads through start(); this
    // covers the Writer/Reader overloads' own wiring branches inside
    // start() (setOutput(Writer)/setError(Writer)/setInput(Reader)), not
    // just the setter's own field assignment.
    StringWriter capturedOut = new StringWriter();
    StringWriter capturedErr = new StringWriter();
    SubInterpreterBuilder builder = SubInterpreterBuilder.elevated()
            .setOutput(capturedOut)
            .setError(capturedErr)
            .setInput(new StringReader("42\n"));
    SubInterpreter sub = startOrSkip(builder);
    try
    {
      Script script = new Script(sub);
      script.exec("import sys; print('out', end=''); print('err', end='', file=sys.stderr)");
      script.exec("import sys; sys.stdout.flush(); sys.stderr.flush()");
      assertEquals(capturedOut.toString(), "out");
      assertEquals(capturedErr.toString(), "err");
      assertEquals(script.eval("input()").toString(), "42");
    } finally
    {
      sub.close();
    }
  }

  @Test
  public void testStartWiresOutputStreamErrorAndInputStreamInput()
  {
    // testSetOutputCapturesPrint already wires an OutputStream for `out`
    // through start(); this covers the OutputStream overload's own wiring
    // branches for `err` and `in` specifically (setError(OutputStream)/
    // setInput(InputStream)), which testStartWiresWriterOutputErrorAndReaderInput
    // deliberately used the Writer/Reader overloads for instead.
    ByteArrayOutputStream capturedErr = new ByteArrayOutputStream();
    SubInterpreterBuilder builder = SubInterpreterBuilder.elevated()
            .setError(capturedErr)
            .setInput(new java.io.ByteArrayInputStream("42\n".getBytes(StandardCharsets.UTF_8)));
    SubInterpreter sub = startOrSkip(builder);
    try
    {
      Script script = new Script(sub);
      script.exec("import sys; print('err', end='', file=sys.stderr); sys.stderr.flush()");
      assertEquals(new String(capturedErr.toByteArray(), StandardCharsets.UTF_8), "err");
      assertEquals(script.eval("input()").toString(), "42");
    } finally
    {
      sub.close();
    }
  }

  @Test
  public void testTryWithResourcesClosesSubInterpreter()
  {
    // SubInterpreter implements Interpreter extends AutoCloseable - no
    // special-casing needed for try-with-resources, it's a plain language
    // guarantee for anything AutoCloseable.
    SubInterpreter captured;
    try (SubInterpreter sub = startOrSkip(SubInterpreterBuilder.elevated()))
    {
      captured = sub;
      assertEquals(new Script(sub).eval("1 + 1").toString(), "2");
    }
    assertFalse(captured.isStarted());
    assertFalse(NativeLauncherControl.isGilHeld());
  }
}
