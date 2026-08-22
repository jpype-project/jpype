// --- file: org/jpype/MainInterpreterDispatchNGTest.java ---
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
package org.jpype;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import org.testng.annotations.Test;
import python.exceptions.PySystemExit;
import python.lang.PyObject;
import python.lang.PyTestHarness;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertNotNull;
import static org.testng.Assert.assertSame;
import static org.testng.Assert.assertTrue;
import static org.testng.Assert.fail;

/**
 * Exercises {@code MainInterpreter.dispatch} (plan/PythonCLI.md decision #2):
 * the {@code -c}/{@code -m}/bare-file/{@code -i} argv dispatch built on top
 * of {@link Runner}.
 */
public class MainInterpreterDispatchNGTest extends PyTestHarness
{

  @Test
  public void testDashCRunsCommand()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    interpreter.dispatch(new String[]
    {
      "-c", "import sys; sys.stdout.write('')"
    });
    // Interpreter must still be usable afterward.
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testDashMRunsModule()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    // "this" is the cheap stdlib Zen-of-Python easter-egg module used
    // elsewhere in RunnerNGTest - no external dependency, observable only
    // via its stdout side effect, safe to run unconditionally.
    interpreter.dispatch(new String[]
    {
      "-m", "this"
    });
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testBareFileRunsScript()
  throws IOException
  {
    Path script = Files.createTempFile("jpype-dispatch-test", ".py");
    try
    {
      Files.write(script, "_dispatch_result = 6 * 7\n".getBytes(StandardCharsets.UTF_8));
      MainInterpreter interpreter = MainInterpreter.getInstance();
      interpreter.dispatch(new String[]
      {
        script.toString()
      });
      assertEquals(context.eval("1+1").toString(), "2");
    } finally
    {
      Files.deleteIfExists(script);
    }
  }

  @Test
  public void testDashCMissingArgumentThrows()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    try
    {
      interpreter.dispatch(new String[]
      {
        "-c"
      });
      fail("expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
    }
  }

  @Test
  public void testDashMMissingArgumentThrows()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    try
    {
      interpreter.dispatch(new String[]
      {
        "-m"
      });
      fail("expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
    }
  }

  @Test
  public void testDashCSystemExitPropagates()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    try
    {
      interpreter.dispatch(new String[]
      {
        "-c", "import sys; sys.exit(2)"
      });
      fail("expected PySystemExit");
    } catch (PySystemExit expected)
    {
    }
    assertEquals(context.eval("1+1").toString(), "2");
  }

  /**
   * Exercises the actual {@code main(String[])} CLI entry point, not just
   * {@code dispatch}. Safe to call here because {@code start()} is
   * idempotent: {@code PyTestHarness.setUpClass} has already started the
   * singleton interpreter, so {@code main}'s call to {@code start(args)}
   * hits the {@code backend != null} early-return and only its
   * {@code dispatch(args)} call actually does anything - equivalent to
   * running {@code python -c ...} from the real CLI without spinning up a
   * second interpreter or process.
   */
  @Test
  public void testMainEntryPointDispatchesDashC()
  {
    MainInterpreter.main(new String[]
    {
      "-c", "_main_entry_result = 6 * 7"
    });
    // main() doesn't return the executed namespace (unlike
    // Runner.runCommand), so just confirm the interpreter is still alive
    // and usable afterward.
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testGetInstallerReturnsSpiInstallerAfterStart()
  {
    // PyTestHarness.setUpClass() has already started the real embedded
    // interpreter for this suite, and the native bridge always calls
    // MainInterpreter.setInstaller(...) (from _jbridge.py's initialize())
    // as part of that startup - so by the time any @Test method runs, a
    // real Installer must be registered.
    MainInterpreter interpreter = MainInterpreter.getInstance();
    Installer installer = interpreter.getInstaller();
    assertNotNull(installer, "installer should be set once the interpreter has started");
    // Repeated calls just return the same registered singleton.
    assertSame(interpreter.getInstaller(), installer);
  }

  @Test
  public void testGetModulePathsReturnsLiveList()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    List<String> paths = interpreter.getModulePaths();
    assertNotNull(paths);
    // No defensive copy is made - callers observe (and can mutate) the
    // interpreter's actual module-path list.
    assertSame(interpreter.getModulePaths(), paths);

    int before = paths.size();
    paths.add("some/extra/module/path");
    try
    {
      assertTrue(interpreter.getModulePaths().contains("some/extra/module/path"));
      assertEquals(interpreter.getModulePaths().size(), before + 1);
    } finally
    {
      paths.remove("some/extra/module/path");
    }
  }
}
