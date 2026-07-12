// --- file: python/lang/InterpreterStdioNGTest.java ---
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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import org.jpype.MainInterpreter;
import org.jpype.Script;
import org.jpype.SubInterpreter;
import org.testng.SkipException;
import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;

/**
 * Exercises {@code Interpreter.setOutput}/{@code setError}/{@code setInput}
 * (plan/StreamRedirect.md): redirecting a Python interpreter's stdio to a
 * plain Java stream via the {@code toPython()} customizers in
 * {@code jpype/_jio.py}, and restoring it via {@code resetOutput}/
 * {@code resetError}/{@code resetInput}.
 */
public class InterpreterStdioNGTest extends PyTestHarness
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
  public void testSetOutputCapturesPrint()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    ByteArrayOutputStream captured = new ByteArrayOutputStream();
    try
    {
      interpreter.setOutput(captured);
      context.exec("print('hello stdout', end='')");
      context.exec("import sys; sys.stdout.flush()");
      assertEquals(new String(captured.toByteArray(), StandardCharsets.UTF_8), "hello stdout");
    } finally
    {
      interpreter.resetOutput();
    }
  }

  @Test
  public void testResetOutputRestoresOriginal()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    ByteArrayOutputStream captured = new ByteArrayOutputStream();
    interpreter.setOutput(captured);
    interpreter.resetOutput();
    // After reset, sys.stdout must be back to the real __stdout__ object -
    // no more writes should land in our captured buffer.
    context.exec("print('after reset', end='')");
    context.exec("import sys; sys.stdout.flush()");
    assertEquals(captured.toByteArray().length, 0);
  }

  @Test
  public void testSetErrorCapturesStderrWrite()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    ByteArrayOutputStream captured = new ByteArrayOutputStream();
    try
    {
      interpreter.setError(captured);
      context.exec("import sys; sys.stderr.write('boom'); sys.stderr.flush()");
      assertEquals(new String(captured.toByteArray(), StandardCharsets.UTF_8), "boom");
    } finally
    {
      interpreter.resetError();
    }
  }

  @Test
  public void testSetInputFeedsReadline()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    ByteArrayInputStream fed = new ByteArrayInputStream(
            "first line\nsecond line\n".getBytes(StandardCharsets.UTF_8));
    try
    {
      interpreter.setInput(fed);
      context.exec("import sys");
      assertEquals(context.eval("sys.stdin.readline()").toString(), "first line\n");
      assertEquals(context.eval("sys.stdin.readline()").toString(), "second line\n");
    } finally
    {
      interpreter.resetInput();
    }
  }

  @Test(timeOut = 15000)
  public void testTwoSubInterpretersRedirectedIndependently()
  {
    SubInterpreter sub1 = startOrSkip();
    SubInterpreter sub2 = startOrSkip();
    ByteArrayOutputStream out1 = new ByteArrayOutputStream();
    ByteArrayOutputStream out2 = new ByteArrayOutputStream();
    try
    {
      Script script1 = new Script(sub1);
      Script script2 = new Script(sub2);

      sub1.setOutput(out1);
      sub2.setOutput(out2);

      script1.exec("print('from sub1', end='')");
      script1.exec("import sys; sys.stdout.flush()");
      script2.exec("print('from sub2', end='')");
      script2.exec("import sys; sys.stdout.flush()");

      assertEquals(new String(out1.toByteArray(), StandardCharsets.UTF_8), "from sub1");
      assertEquals(new String(out2.toByteArray(), StandardCharsets.UTF_8), "from sub2");
    } finally
    {
      sub1.close();
      sub2.close();
    }
  }
}
