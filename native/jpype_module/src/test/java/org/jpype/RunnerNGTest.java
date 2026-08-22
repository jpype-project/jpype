// --- file: org/jpype/RunnerNGTest.java ---
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
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import org.testng.annotations.Test;
import python.exceptions.PySystemExit;
import python.lang.PyDict;
import python.lang.PyObject;
import python.lang.PyTestHarness;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;
import static org.testng.Assert.fail;

/**
 * Exercises {@code org.jpype.Runner} (plan/PythonCLI.md): running a module,
 * a script file, and an inline command the way the real {@code python}
 * command line would, plus the {@code sys.argv} save/restore guarantee.
 */
public class RunnerNGTest extends PyTestHarness
{

  @Test
  public void testRunModuleRunsAndLeavesInterpreterUsable()
  {
    Runner runner = new Runner(MainInterpreter.getInstance());
    // "this" is the stdlib Zen-of-Python easter egg module - cheap, has an
    // observable side effect (prints to stdout), no external dependency.
    runner.runModule("this");
    // Interpreter must still be usable afterward.
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testRunCommandSetsArgv0ToDashC()
  {
    Runner runner = new Runner(MainInterpreter.getInstance());
    PyDict ns = runner.runCommand("import sys; _argv0 = sys.argv[0]");
    assertEquals(ns.get("_argv0").toString(), "-c");
  }

  @Test
  public void testRunCommandArgsAppendedToArgv()
  {
    Runner runner = new Runner(MainInterpreter.getInstance());
    PyDict ns = runner.runCommand("import sys; _argv = list(sys.argv)", "foo", "bar");
    assertEquals(ns.get("_argv").toString(), "['-c', 'foo', 'bar']");
  }

  @Test
  public void testArgvRestoredAfterRunCommand()
  {
    Runner runner = new Runner(MainInterpreter.getInstance());
    PyObject before = context.eval("__import__('sys').argv");
    String beforeStr = before.toString();
    runner.runCommand("1+1");
    PyObject after = context.eval("__import__('sys').argv");
    assertEquals(after.toString(), beforeStr);
  }

  @Test
  public void testArgvRestoredEvenWhenCommandRaises()
  {
    Runner runner = new Runner(MainInterpreter.getInstance());
    PyObject before = context.eval("__import__('sys').argv");
    String beforeStr = before.toString();
    try
    {
      runner.runCommand("raise ValueError('boom')");
      fail("expected an exception");
    } catch (RuntimeException expected)
    {
      // expected - a plain Python exception raised from the command.
    }
    PyObject after = context.eval("__import__('sys').argv");
    assertEquals(after.toString(), beforeStr);
  }

  @Test
  public void testRunFileRunsScriptWithArgv()
  throws IOException
  {
    Path script = Files.createTempFile("jpype-runner-test", ".py");
    try
    {
      Files.write(script,
              ("import sys\n"
              + "_argv = list(sys.argv)\n"
              + "_result = 6 * 7\n").getBytes(java.nio.charset.StandardCharsets.UTF_8));
      Runner runner = new Runner(MainInterpreter.getInstance());
      PyDict ns = runner.runFile(script, "arg1");
      assertEquals(ns.get("_result").toString(), "42");
      assertEquals(ns.get("_argv").toString(), "['" + script.toString() + "', 'arg1']");
    } finally
    {
      Files.deleteIfExists(script);
    }
  }

  @Test
  public void testSystemExitFromCommandSurfacesAsPySystemExit()
  {
    Runner runner = new Runner(MainInterpreter.getInstance());
    try
    {
      runner.runCommand("import sys; sys.exit(3)");
      fail("expected PySystemExit");
    } catch (PySystemExit expected)
    {
      assertTrue(expected.get() != null);
    }
    // Interpreter must still be usable afterward.
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testPipInstallIsRunModulePipWithArgs()
  {
    // Don't actually hit the network / pip in this test - just confirm the
    // dispatch shape: pipInstall("list") should behave like `python -m pip
    // list`. `pip list` touches no network and is safe to run
    // unconditionally. Real pip's __main__ always calls sys.exit(), even on
    // success (exit code 0) - so a clean run surfaces as PySystemExit(0),
    // not a normal return. That's correct behavior, not a bug: it's the
    // same SystemExit translation exercised in
    // testSystemExitFromCommandSurfacesAsPySystemExit, just reached via
    // pipInstall/runModule instead of runCommand.
    Runner runner = new Runner(MainInterpreter.getInstance());
    try
    {
      runner.pipInstall("list");
      fail("expected PySystemExit(0), matching real `pip list`'s own sys.exit(0)");
    } catch (PySystemExit expected)
    {
      // expected.
    }
    assertEquals(context.eval("1+1").toString(), "2");
  }
}
